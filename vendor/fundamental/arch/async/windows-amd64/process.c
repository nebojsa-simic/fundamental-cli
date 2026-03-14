#include "async/async.h"
#include "string/string.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static AsyncStatus read_pipe_to_buffer(HANDLE hPipe, uint8_t *buffer,
									   size_t buffer_size, size_t *write_pos,
									   size_t *read_pos, size_t *count)
{
	DWORD bytes_available = 0;
	if (!PeekNamedPipe(hPipe, NULL, 0, NULL, &bytes_available, NULL)) {
		return ASYNC_ERROR;
	}

	if (bytes_available == 0) {
		return ASYNC_PENDING;
	}

	DWORD bytes_to_read = bytes_available < buffer_size ? bytes_available :
														  buffer_size;
	DWORD bytes_read = 0;

	if (ReadFile(hPipe, buffer, bytes_to_read, &bytes_read, NULL)) {
		for (DWORD i = 0; i < bytes_read; i++) {
			if (*count >= buffer_size) {
				*read_pos = (*read_pos + 1) % buffer_size;
			} else {
				buffer[*write_pos] = buffer[i];
				*write_pos = (*write_pos + 1) % buffer_size;
				(*count)++;
			}
		}
	}

	return ASYNC_PENDING;
}

AsyncResult platform_process_spawn(AsyncResult *result, const char *executable,
								   const char **args,
								   const ProcessSpawnOptions *options)
{
	(void)options;

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE stdout_read = NULL;
	HANDLE stdout_write = NULL;
	HANDLE stderr_read = NULL;
	HANDLE stderr_write = NULL;

	if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
		result->status = ASYNC_ERROR;
		result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										 "Failed to create stdout pipe");
		return *result;
	}

	if (!CreatePipe(&stderr_read, &stderr_write, &sa, 0)) {
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		result->status = ASYNC_ERROR;
		result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										 "Failed to create stderr pipe");
		return *result;
	}

	if (!SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		CloseHandle(stderr_read);
		CloseHandle(stderr_write);
		result->status = ASYNC_ERROR;
		result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										 "Failed to set stdout handle info");
		return *result;
	}

	if (!SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		CloseHandle(stderr_read);
		CloseHandle(stderr_write);
		result->status = ASYNC_ERROR;
		result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
										 "Failed to set stderr handle info");
		return *result;
	}

	char cmd_line[4096] = { 0 };
	cmd_line[0] = '"';
	fun_string_copy(executable, cmd_line + 1);
	StringLength exe_len = fun_string_length(executable);
	cmd_line[1 + exe_len] = '"';
	cmd_line[2 + exe_len] = '\0';
	if (args != NULL) {
		for (int i = 0; args[i] != NULL; i++) {
			StringLength cur_len = fun_string_length((String)cmd_line);
			cmd_line[cur_len] = ' ';
			fun_string_copy((String)args[i], cmd_line + cur_len + 1);
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
		DWORD error = GetLastError();
		CloseHandle(stdout_read);
		CloseHandle(stdout_write);
		CloseHandle(stderr_read);
		CloseHandle(stderr_write);

		result->status = ASYNC_ERROR;
		if (error == ERROR_FILE_NOT_FOUND) {
			result->error = fun_error_result(ERROR_CODE_PROCESS_NOT_FOUND,
											 "Executable not found");
		} else {
			result->error = fun_error_result(ERROR_CODE_PROCESS_SPAWN_FAILED,
											 "Failed to create process");
		}
		return *result;
	}

	CloseHandle(stdout_write);
	CloseHandle(stderr_write);

	result->process.handle = pi.hProcess;
	result->process.stdout_pipe = stdout_read;
	result->process.stderr_pipe = stderr_read;
	result->process.is_running = 1;
	result->process.platform_data[0] = (int)(intptr_t)pi.hThread;

	return *result;
}

AsyncStatus platform_process_poll(AsyncResult *result)
{
	if (result == NULL || result->process.handle == NULL) {
		return ASYNC_ERROR;
	}

	DWORD exit_code = 0;
	if (!GetExitCodeProcess(result->process.handle, &exit_code)) {
		return ASYNC_ERROR;
	}

	if (exit_code != STILL_ACTIVE) {
		result->process.is_running = 0;
		result->process.exit_code = (int)exit_code;
		result->status = ASYNC_COMPLETED;

		HANDLE stdout_pipe = (HANDLE)result->process.stdout_pipe;
		HANDLE stderr_pipe = (HANDLE)result->process.stderr_pipe;

		if (stdout_pipe != NULL) {
			read_pipe_to_buffer(stdout_pipe, result->process.stdout_buffer,
								PROCESS_STDOUT_BUFFER_SIZE,
								&result->process.stdout_write_pos,
								&result->process.stdout_read_pos,
								&result->process.stdout_count);
			CloseHandle(stdout_pipe);
			result->process.stdout_pipe = NULL;
		}

		if (stderr_pipe != NULL) {
			read_pipe_to_buffer(stderr_pipe, result->process.stderr_buffer,
								PROCESS_STDERR_BUFFER_SIZE,
								&result->process.stderr_write_pos,
								&result->process.stderr_read_pos,
								&result->process.stderr_count);
			CloseHandle(stderr_pipe);
			result->process.stderr_pipe = NULL;
		}

		CloseHandle(result->process.handle);
		result->process.handle = NULL;

		HANDLE hThread = (HANDLE)(intptr_t)result->process.platform_data[0];
		if (hThread != NULL) {
			CloseHandle(hThread);
			result->process.platform_data[0] = 0;
		}

		return ASYNC_COMPLETED;
	}

	HANDLE stdout_pipe = (HANDLE)result->process.stdout_pipe;
	HANDLE stderr_pipe = (HANDLE)result->process.stderr_pipe;

	if (stdout_pipe != NULL) {
		read_pipe_to_buffer(stdout_pipe, result->process.stdout_buffer,
							PROCESS_STDOUT_BUFFER_SIZE,
							&result->process.stdout_write_pos,
							&result->process.stdout_read_pos,
							&result->process.stdout_count);
	}

	if (stderr_pipe != NULL) {
		read_pipe_to_buffer(stderr_pipe, result->process.stderr_buffer,
							PROCESS_STDERR_BUFFER_SIZE,
							&result->process.stderr_write_pos,
							&result->process.stderr_read_pos,
							&result->process.stderr_count);
	}

	return ASYNC_PENDING;
}

ErrorResult platform_process_terminate(Process *process)
{
	if (process == NULL || process->handle == NULL) {
		return ERROR_RESULT_NO_ERROR;
	}

	if (!TerminateProcess(process->handle, 1)) {
		return fun_error_result(ERROR_CODE_PROCESS_TERMINATE_FAILED,
								"Failed to terminate process");
	}

	process->is_running = 0;
	return ERROR_RESULT_NO_ERROR;
}

ErrorResult platform_process_free(Process *process)
{
	if (process == NULL) {
		return ERROR_RESULT_NULL_POINTER;
	}

	if (process->handle != NULL) {
		CloseHandle(process->handle);
		process->handle = NULL;
	}

	if (process->stdout_pipe != NULL) {
		CloseHandle(process->stdout_pipe);
		process->stdout_pipe = NULL;
	}

	if (process->stderr_pipe != NULL) {
		CloseHandle(process->stderr_pipe);
		process->stderr_pipe = NULL;
	}

	HANDLE hThread = (HANDLE)(intptr_t)process->platform_data[0];
	if (hThread != NULL) {
		CloseHandle(hThread);
		process->platform_data[0] = 0;
	}

	process->is_running = 0;
	return ERROR_RESULT_NO_ERROR;
}
