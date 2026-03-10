#include "cmd_init.h"
#include "src/cli.h"
#include "vendor/fundamental/include/console/console.h"
#include "vendor/fundamental/include/memory/memory.h"
#include "vendor/fundamental/include/string/string.h"
#include "vendor/fundamental/include/file/file.h"
#include "vendor/fundamental/include/async/async.h"
#include "vendor/fundamental/include/filesystem/filesystem.h"

static ErrorResult write_file(const char* path, const char* content) {
  StringLength len = fun_string_length(content);
  MemoryResult mem_result = fun_memory_allocate(len);
  if (fun_error_is_error(mem_result.error)) return mem_result.error;
  fun_string_copy(content, (char*)mem_result.value);
  AsyncResult write_result = fun_write_memory_to_file((Write){.file_path = (String)path, .input = mem_result.value, .bytes_to_write = len});
  fun_async_await(&write_result);
  voidResult free_result = fun_memory_free(&mem_result.value);
  (void)free_result;
  return (write_result.status != ASYNC_COMPLETED) ? fun_error_result(1, "Failed") : ERROR_RESULT_NO_ERROR;
}

int cmd_init_execute(int argc, const char **argv) {
  (void)argc; (void)argv;
  fun_console_write_line("Initializing...\n");
  fun_filesystem_create_directory((String)"src");
  fun_console_write_line("Dirs created\n");
  write_file("test.txt", "Hello!\n");
  fun_console_write_line("File written\n");
  fun_console_write_line("Done!\n");
  return 0;
}
