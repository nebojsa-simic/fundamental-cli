# Async File Write Test Case

## Bug Description

When `fun init` command runs, it hangs after creating directories. The issue is in the `write_file()` function in `commands/cmd_init.c` which uses `fun_write_memory_to_file()` followed by `fun_async_await()`.

## Expected Behavior

The async file write should complete quickly and files should be written to disk.

## Actual Behavior

The `fun_async_await()` call hangs indefinitely, never returning from the polling loop.

## Test Case

This test (`test.c`) reproduces the async file write in isolation:

```c
AsyncResult write_result = fun_write_memory_to_file((Write){
    .file_path = "test_output.txt",
    .input = mem_result.value,
    .bytes_to_write = len
});

fun_async_await(&write_result);  // Hangs here in cmd_init.c, but works in this test!
```

## Results

✅ **This test PASSES** - async file write completes successfully
❌ **cmd_init.c FAILS** - same code pattern hangs

## Key Difference

The test case uses simple, small strings (~35 bytes).
The `cmd_init.c` uses LARGE static const strings (thousands of bytes for templates).

## Suspected Root Cause

The large static string templates in `cmd_init.c` may be causing:
1. Stack overflow issues (hence the `___chkstk_ms` errors)
2. Memory alignment issues
3. The async state object may be getting corrupted

## Next Steps

1. Compare the Memory objects created in test.c vs cmd_init.c
2. Check if the poll function receives valid state in both cases
3. Add debug output to `poll_mmap_write` to see what's happening
4. Consider breaking up the large template strings into smaller chunks

## How to Run

```batch
cd tests/asyncFileWrite
build-windows-amd64.bat
```

Expected output:
```
Testing async file write...
✓ Memory allocated and content copied
✓ Async operation initiated
✓ Polling for completion...
✓ Async operation completed
✓ File written successfully
All tests passed!
```
