#include <windows.h>
#include "file/file.h"
#include "memory/memory.h"
#include "error/error.h"

ErrorResult fun_lock_file(String filePath, FileLockHandle *outLockHandle)
{
	if (!filePath || !outLockHandle) {
		return ERROR_RESULT_NULL_POINTER;
	}

	// Convert UTF8 path to wide string for Windows
	wchar_t wide_path[MAX_PATH];
	int converted =
		MultiByteToWideChar(CP_UTF8, 0, filePath, -1, wide_path, MAX_PATH);
	if (converted == 0) {
		return fun_error_result(GetLastError(), "Failed to convert file path");
	}

	HANDLE file_handle =
		CreateFileW(wide_path,
					GENERIC_READ | GENERIC_WRITE, // Request access to the file
					0, // No sharing - exclusive access
					NULL, // Security attributes
					OPEN_EXISTING, // Open existing file only
					FILE_ATTRIBUTE_NORMAL, // File attributes
					NULL // Template file
		);

	if (file_handle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		return fun_error_result(error, "Failed to acquire file lock");
	}

	// Store the handle in the lock handle for later unlock
	outLockHandle->state = (void *)file_handle;

	return ERROR_RESULT_NO_ERROR;
}

ErrorResult fun_unlock_file(FileLockHandle lockHandle)
{
	if (!lockHandle.state) {
		return ERROR_RESULT_NULL_POINTER;
	}

	HANDLE file_handle = (HANDLE)lockHandle.state;
	BOOL result = CloseHandle(file_handle);

	if (!result) {
		DWORD error = GetLastError();
		return fun_error_result(error, "Failed to release file lock");
	}

	return ERROR_RESULT_NO_ERROR;
}