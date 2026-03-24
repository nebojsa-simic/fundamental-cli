#include "fundamental/process/process.h"
#include "fundamental/string/string.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* ------------------------------------------------------------------
 * Internal handle layout stored in ProcessResult->_handle.
 * We allocate a small struct on the heap — the only internal allocation
 * in this module, necessary to hold multiple OS handles across calls.
 * ------------------------------------------------------------------ */
typedef struct {
	HANDLE hProcess;
	HANDLE hThread;
	HANDLE hStdout;
	HANDLE hStderr;
} WinProcHandles;

static AsyncStatus win_process_poll(AsyncResult *result)
{
	ProcessResult *out = (ProcessResult *)result->state;
	if (out == NULL || out->_handle == NULL) {
		return ASYNC_ERROR;
	}

	WinProcHandles *h = (WinProcHandles *)out->_handle;

	DWORD exit_code = 0;
	if (!GetExitCodeProcess(h->hProcess, &exit_code)) {
		return ASYNC_ERROR;
	}

	/* Drain pipes on every poll to prevent the child from blocking on a full
	 * pipe buffer while the parent waits for it to exit (classic deadlock). */
	if (h->hStdout != NULL && out->stdout_data != NULL) {
		DWORD bytes_read = 0;
		DWORD available = 0;
		while (PeekNamedPipe(h->hStdout, NULL, 0, NULL, &available, NULL) &&
			   available > 0 && out->stdout_length < out->stdout_capacity) {
			DWORD to_read = (DWORD)(out->stdout_capacity - out->stdout_length);
			if (to_read > available)
				to_read = available;
			if (!ReadFile(h->hStdout, out->stdout_data + out->stdout_length,
						  to_read, &bytes_read, NULL))
				break;
			out->stdout_length += bytes_read;
		}
	}

	if (h->hStderr != NULL && out->stderr_data != NULL) {
		DWORD bytes_read = 0;
		DWORD available = 0;
		while (PeekNamedPipe(h->hStderr, NULL, 0, NULL, &available, NULL) &&
			   available > 0 && out->stderr_length < out->stderr_capacity) {
			DWORD to_read = (DWORD)(out->stderr_capacity - out->stderr_length);
			if (to_read > available)
				to_read = available;
			if (!ReadFile(h->hStderr, out->stderr_data + out->stderr_length,
						  to_read, &bytes_read, NULL))
				break;
			out->stderr_length += bytes_read;
		}
	}

	if (exit_code == STILL_ACTIVE) {
		return ASYNC_PENDING;
	}

	/* Process has exited — close pipe handles */
	out->exit_code = (int)exit_code;

	if (h->hStdout != NULL) {
		CloseHandle(h->hStdout);
		h->hStdout = NULL;
	}
	if (h->hStderr != NULL) {
		CloseHandle(h->hStderr);
		h->hStderr = NULL;
	}

	result->status = ASYNC_COMPLETED;
	return ASYNC_COMPLETED;
}

AsyncResult fun_process_arch_spawn(const char *executable, const char **args,
								   const ProcessSpawnOptions *options,
								   ProcessResult *out)
{
	(void)options;

	AsyncResult result;
	result.poll = win_process_poll;
	result.state = out;
	result.status = ASYNC_PENDING;
	result.error = ERROR_RESULT_NO_ERROR;

	/* Reset output fields */
	out->stdout_length = 0;
	out->stderr_length = 0;
	out->exit_code = 0;

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	HANDLE stdout_read = NULL, stdout_write = NULL;
	HANDLE stderr_read = NULL, stderr_write = NULL;

	if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stdout pipe");
		return result;
	}
	if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to create stderr pipe");
		return result;
	}

	SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);

	/* Build command line */
	char cmd_line[4096] = { 0 };
	cmd_line[0] = '"';
	voidResult copy_r =
		fun_string_copy(executable, cmd_line + 1, sizeof(cmd_line) - 1);
	if (fun_error_is_error(copy_r.error)) {
		result.status = ASYNC_ERROR;
		result.error = copy_r.error;
		return result;
	}
	StringLength exe_len = fun_string_length(executable);
	cmd_line[1 + exe_len] = '"';
	cmd_line[2 + exe_len] = '\0';
	if (args != NULL) {
		for (int i = 1; args[i] != NULL; i++) {
			StringLength cur_len = fun_string_length((String)cmd_line);
			if (cur_len + 2 >= sizeof(cmd_line)) {
				result.status = ASYNC_ERROR;
				result.error = fun_error_result(ERROR_CODE_BUFFER_TOO_SMALL,
												"Command line too long");
				return result;
			}
			cmd_line[cur_len] = ' ';
			copy_r = fun_string_copy((String)args[i], cmd_line + cur_len + 1,
									 sizeof(cmd_line) - cur_len - 1);
			if (fun_error_is_error(copy_r.error)) {
				result.status = ASYNC_ERROR;
				result.error = copy_r.error;
				return result;
			}
		}
	}

	STARTUPINFOA si = { 0 };
	si.cb = sizeof(STARTUPINFOA);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = stdout_write;
	si.hStdError = stderr_write;

	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessA(NULL, cmd_line, NULL, NULL, TRUE, 0, NULL, NULL, &si,
						&pi)) {
		DWORD err = GetLastError();
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		CloseHandle(stderr_read);
		CloseHandle(stderr_write);
		result.status = ASYNC_ERROR;
		if (err == ERROR_FILE_NOT_FOUND) {
			result.error = fun_error_result(ERROR_CODE_PROCESS_NOT_FOUND,
											"Executable not found");
		} else {
			result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
											"Failed to create process");
		}
		return result;
	}

	CloseHandle(stdout_write);
	CloseHandle(stderr_write);

	/* Store handles — use caller's _handle field with a stack-local struct.
	 * We allocate with LocalAlloc (Windows heap, no stdlib) to persist
	 * across poll calls. */
	WinProcHandles *h =
		(WinProcHandles *)LocalAlloc(LMEM_ZEROINIT, sizeof(WinProcHandles));
	if (h == NULL) {
		TerminateProcess(pi.hProcess, 1);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(stdout_read);
		CloseHandle(stderr_read);
		result.status = ASYNC_ERROR;
		result.error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										"Failed to allocate process handles");
		return result;
	}

	h->hProcess = pi.hProcess;
	h->hThread = pi.hThread;
	h->hStdout = stdout_read;
	h->hStderr = stderr_read;
	out->_handle = h;

	return result;
}

voidResult fun_process_arch_terminate(ProcessResult *out)
{
	voidResult r;
	r.error = ERROR_RESULT_NO_ERROR;

	if (out->_handle == NULL) {
		return r;
	}

	WinProcHandles *h = (WinProcHandles *)out->_handle;
	if (h->hProcess != NULL) {
		if (!TerminateProcess(h->hProcess, 1)) {
			r.error = fun_error_result(ERROR_CODE_PROCESS_TERMINATE_FAILED,
									   "Failed to terminate process");
		}
	}
	return r;
}

voidResult fun_process_arch_free(ProcessResult *out)
{
	voidResult r;
	r.error = ERROR_RESULT_NO_ERROR;

	if (out->_handle == NULL) {
		return r;
	}

	WinProcHandles *h = (WinProcHandles *)out->_handle;

	if (h->hStdout != NULL) {
		CloseHandle(h->hStdout);
		h->hStdout = NULL;
	}
	if (h->hStderr != NULL) {
		CloseHandle(h->hStderr);
		h->hStderr = NULL;
	}
	if (h->hThread != NULL) {
		CloseHandle(h->hThread);
		h->hThread = NULL;
	}
	if (h->hProcess != NULL) {
		CloseHandle(h->hProcess);
		h->hProcess = NULL;
	}

	LocalFree(h);
	out->_handle = NULL;
	return r;
}
