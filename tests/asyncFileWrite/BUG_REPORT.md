# Bug Report: Async File Write Hangs in cmd_init.c

## Summary

The `fun init` command hangs when trying to write files using `fun_write_memory_to_file()` and `fun_async_await()`, even though a minimal test case with the same pattern works perfectly.

## Test Results

### ✅ test.c (PASSES)
- Location: `tests/asyncFileWrite/test.c`
- Writes small string (35 bytes)
- Async operation completes successfully
- File created on disk

### ❌ cmd_init.c (FAILS)
- Location: `commands/cmd_init.c`
- Writes large template strings (hundreds of bytes each)
- Async operation hangs in `fun_async_await()`
- No files created

## Reproduction Steps

1. Build fun.exe: `.\build-windows-amd64.bat`
2. Run in clean directory: `mkdir test && cd test && ..\fun.exe init`
3. Observe: Hangs after "✓ Created directory structure"

## Code Comparison

### Working (test.c):
```c
const char* test_content = "Hello, World!\nThis is a test file.\n";  // 35 bytes
StringLength len = fun_string_length(test_content);
MemoryResult mem_result = fun_memory_allocate(len);
fun_string_copy(test_content, (char*)mem_result.value);
AsyncResult write_result = fun_write_memory_to_file((Write){
    .file_path = "test_output.txt",
    .input = mem_result.value,
    .bytes_to_write = len
});
fun_async_await(&write_result);  // ✓ Completes
```

### Not Working (cmd_init.c):
```c
static const char* TEMPLATE_MAIN_C = "#include \"commands/cmd_version.h\"\n...";  // ~500 bytes
// ... same pattern as test.c ...
fun_async_await(&write_result);  // ✗ Hangs
```

## Hypothesis

The large static const strings in cmd_init.c may be causing:
1. **Stack issues**: Large local variables triggering stack probe
2. **Memory corruption**: The Memory object or AsyncResult state getting corrupted
3. **Poll function issue**: `poll_mmap_write` not receiving valid state pointer

## Recommended Debugging Steps

1. **Add debug output to poll_mmap_write** in `vendor/fundamental/arch/file/windows-amd64/fileWriteMmap.c`:
   ```c
   fun_console_write_line("poll_mmap_write called\n");
   ```

2. **Check Memory allocation**: Verify mem_result.value is valid before calling write

3. **Try smaller templates**: Break up large templates to see if size is the issue

4. **Check state pointer**: Add debug in poll function to print state pointer value

## Files to Investigate

- `commands/cmd_init.c` - lines 144-170 (write_file function)
- `vendor/fundamental/arch/file/windows-amd64/fileWriteMmap.c` - poll_mmap_write function
- `vendor/fundamental/src/async/async.c` - fun_async_await function

## Test Command

```batch
cd tests\asyncFileWrite
build-windows-amd64.bat
```

Should output: "All tests passed!"
