// Test: Async File Write Completion
// Reproduces bug where fun_write_memory_to_file never completes
// Expected: File should be written and async operation should complete
// Actual: fun_async_await hangs indefinitely

#include "../../vendor/fundamental/include/async/async.h"
#include "../../vendor/fundamental/include/console/console.h"
#include "../../vendor/fundamental/include/error/error.h"
#include "../../vendor/fundamental/include/file/file.h"
#include "../../vendor/fundamental/include/memory/memory.h"
#include "../../vendor/fundamental/include/string/string.h"

// Forward declaration - our test is the entry point
int cli_main(void);

int cli_main(void)
{
	fun_console_write_line("Testing async file write...\n");

	// Allocate and prepare test data
	const char *test_content = "Hello, World!\nThis is a test file.\n";
	StringLength len = fun_string_length(test_content);

	MemoryResult mem_result = fun_memory_allocate(len);
	if (fun_error_is_error(mem_result.error)) {
		fun_console_error_line("Failed to allocate memory");
		return 1;
	}

	fun_string_copy(test_content, (char *)mem_result.value);
	fun_console_write_line("✓ Memory allocated and content copied\n");

	// Attempt to write file asynchronously
	fun_console_write_line("Calling fun_write_memory_to_file...\n");
	AsyncResult write_result =
		fun_write_memory_to_file((Write){ .file_path = "test_output.txt",
										  .input = mem_result.value,
										  .bytes_to_write = len });

	fun_console_write_line("✓ Async operation initiated\n");
	fun_console_write_line("Polling for completion...\n");

	// This should complete quickly but hangs
	int poll_count = 0;
	while (write_result.status == ASYNC_PENDING) {
		poll_count++;
		if (poll_count > 1000000) {
			fun_console_error_line(
				"ERROR: Polling timed out after 1000000 iterations");
			fun_console_error_line("Async file write never completed!");
			fun_memory_free(&mem_result.value);
			return 1;
		}
		fun_async_await(&write_result);
	}

	fun_console_write_line("✓ Async operation completed\n");

	// Check result
	if (write_result.status == ASYNC_COMPLETED) {
		fun_console_write_line("✓ File written successfully\n");
	} else if (write_result.status == ASYNC_ERROR) {
		fun_console_error_line("ERROR: File write failed");
		fun_memory_free(&mem_result.value);
		return 1;
	}

	// Cleanup
	voidResult free_result = fun_memory_free(&mem_result.value);
	if (fun_error_is_error(free_result.error)) {
		fun_console_error_line("Failed to free memory");
		return 1;
	}

	fun_console_write_line("\nAll tests passed!\n");
	return 0;
}
