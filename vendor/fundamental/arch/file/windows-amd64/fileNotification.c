#include <windows.h>
#include "fundamental/file/file.h"
#include "fundamental/filesystem/filesystem.h"
#include "fundamental/memory/memory.h"
#include "fundamental/error/error.h"
#include "fundamental/async/async.h"

// Structure to hold state for file change monitoring
typedef struct {
	String file_path;
	FileChangeCallback callback;
	HANDLE dir_handle;
	OVERLAPPED overlapped;
	BYTE buffer[1024]; // Buffer for directory change notifications
	BOOL monitoring_active; // Flag to indicate monitoring state
} FileNotificationState;

// Static variable to control the monitoring thread
static HANDLE monitoring_thread = NULL;
static volatile BOOL monitoring_should_stop = FALSE;

// Thread function to monitor file changes
DWORD WINAPI file_monitor_thread(LPVOID param)
{
	FileNotificationState *state = (FileNotificationState *)param;

	DWORD bytesReturned;
	BOOL result;

	while (!monitoring_should_stop) {
		// Reset the OVERLAPPED operation fields, preserving the event handle
		unsigned char *overlapped_bytes = (unsigned char *)&state->overlapped;
		for (size_t i = 0; i < sizeof(OVERLAPPED) - sizeof(HANDLE); i++)
			overlapped_bytes[i] = 0;

		// Watch for changes to the file
		result = ReadDirectoryChangesW(
			state->dir_handle, state->buffer, sizeof(state->buffer),
			FALSE, // Monitor only specific file (not subtree)
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE |
				FILE_NOTIFY_CHANGE_SIZE,
			&bytesReturned, &(state->overlapped),
			NULL // No completion routine
		);

		if (result) {
			// Wait for change notification
			DWORD waitResult =
				WaitForSingleObjectEx((HANDLE)state->overlapped.hEvent,
									  5000, // 5-second timeout
									  TRUE // Alertable wait
				);

			if (waitResult == WAIT_OBJECT_0 && state->callback) {
				// Process the directory change notification
				FILE_NOTIFY_INFORMATION *notifyInfo =
					(FILE_NOTIFY_INFORMATION *)state->buffer;

				// Get filename from the notification
				wchar_t fileName[256];
				int len = notifyInfo->FileNameLength / sizeof(WCHAR);
				if (len >= 256)
					len = 255;
				fun_memory_copy(notifyInfo->FileName, fileName,
								len * sizeof(WCHAR));
				fileName[len] = L'\0';

				// Convert back to UTF-8 if needed and call the callback
				char utf8_filename[512];
				WideCharToMultiByte(CP_UTF8, 0, fileName, -1, utf8_filename,
									512, NULL, NULL);

				// Call the user-provided callback
				state->callback(utf8_filename);
			}
		} else {
			// On error, sleep briefly and continue
			Sleep(100);
		}
	}

	return 0;
}

// Extract the parent directory of a path using the platform separator.
// Mirrors the Linux inotify implementation's get_parent_dir helper.
static void get_parent_dir(const char *path, char *dir_buf)
{
	size_t len = 0;
	while (path[len] != '\0') {
		dir_buf[len] = path[len];
		len++;
	}
	dir_buf[len] = '\0';

	char sep = fun_path_separator();
	while (len > 0 && dir_buf[len - 1] != sep) {
		len--;
	}

	if (len > 0) {
		dir_buf[len - 1] = '\0';
	} else {
		dir_buf[0] = '.';
		dir_buf[1] = '\0';
	}
}

AsyncResult fun_register_file_change_notification(String filePath,
												  FileChangeCallback callback)
{
	if (!filePath || !callback) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	// Allocate our state variable
	MemoryResult stateResult =
		fun_memory_allocate(sizeof(FileNotificationState));
	if (fun_error_is_error(stateResult.error)) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = stateResult.error };
	}

	FileNotificationState *state = (FileNotificationState *)stateResult.value;
	fun_memory_fill(state, sizeof(FileNotificationState), 0);

	// Initialize event for overlapped operations
	state->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (state->overlapped.hEvent == NULL) {
		fun_memory_free((Memory *)&state);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = fun_error_result(
								  GetLastError(),
								  "Failed to create event handle") };
	}

	// Prepare the path - extract directory path from file path
	char dir_path_utf8[512];
	get_parent_dir(filePath, dir_path_utf8);

	wchar_t dir_path[512];
	int dir_converted =
		MultiByteToWideChar(CP_UTF8, 0, dir_path_utf8, -1, dir_path, 512);
	if (dir_converted == 0) {
		CloseHandle(state->overlapped.hEvent);
		fun_memory_free((Memory *)&state);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = fun_error_result(
								  GetLastError(),
								  "Failed to convert directory path") };
	}

	// Store file details
	state->file_path = filePath;
	state->callback = callback;

	// Open directory handle for monitoring
	state->dir_handle = CreateFileW(
		dir_path, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	if (state->dir_handle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		CloseHandle(state->overlapped.hEvent);
		fun_memory_free((Memory *)&state);
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = fun_error_result(
								  error,
								  "Failed to open directory for monitoring") };
	}

	// Store state in AsyncResult structure and keep monitoring running
	AsyncResult result = {
		.state = state,
		.status = ASYNC_PENDING, // Indicate that monitoring has started
		.error = ERROR_RESULT_NO_ERROR,
	};

	monitoring_should_stop = FALSE;

	// Create the monitoring thread
	DWORD threadId;
	monitoring_thread = CreateThread(NULL, // Security attributes
									 0, // Stack size
									 file_monitor_thread, // Thread function
									 state, // Parameter to thread function
									 0, // Creation flags
									 &threadId // Thread ID
	);

	if (monitoring_thread == NULL) {
		DWORD error = GetLastError();
		CloseHandle(state->dir_handle);
		CloseHandle(state->overlapped.hEvent);
		fun_memory_free((Memory *)&state);
		return (
			AsyncResult){ .status = ASYNC_ERROR,
						  .error = fun_error_result(
							  error, "Failed to create monitoring thread") };
	}

	return result;
}

AsyncResult fun_unregister_file_change_notification(void *state)
{
	if (!state) {
		return (AsyncResult){ .status = ASYNC_ERROR,
							  .error = ERROR_RESULT_NULL_POINTER };
	}

	FileNotificationState *s = (FileNotificationState *)state;

	if (monitoring_thread != NULL) {
		// Signal the monitoring thread to stop
		monitoring_should_stop = TRUE;

		// Give thread some time to finish
		DWORD waitResult =
			WaitForSingleObject(monitoring_thread, 2000); // 2 second timeout

		if (waitResult == WAIT_TIMEOUT) {
			// If thread doesn't stop in time, terminate it (not ideal but necessary)
			TerminateThread(monitoring_thread, 1);
		}

		CloseHandle(monitoring_thread);
		monitoring_thread = NULL;
	}

	if (s->dir_handle != NULL && s->dir_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(s->dir_handle);
	}

	if (s->overlapped.hEvent != NULL) {
		CloseHandle(s->overlapped.hEvent);
	}

	fun_memory_free((Memory *)&state);

	return (AsyncResult){ .status = ASYNC_COMPLETED,
						  .error = ERROR_RESULT_NO_ERROR };
}