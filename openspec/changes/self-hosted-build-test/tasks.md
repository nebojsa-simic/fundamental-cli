## 1. Real Source Scanner

- [x] 1.1 In `generator.c`, increase `script_buffer` static arrays from 4096 to 8192 bytes in both `build_generate_windows()` and `build_generate_linux()`
- [x] 1.2 In `generator.c`, add a `MAX_DIR_LISTING` constant (16384 bytes) and replace `scan_directory_recursive()` with a real implementation that calls `fun_filesystem_list_directory()` to list entries, then uses `fun_path_join()` + `fun_file_exists()` / `fun_directory_exists()` to classify each entry as a `.c` file (include) or subdirectory (recurse), with max recursion depth of 8
- [x] 1.3 Update `generator.h` (or `generator.c`) to remove the now-dead hardcoded path arrays (`common_paths[]` and `fun_paths[]`)

## 2. Binary Name from fun.ini

- [x] 2.1 In `generator.c`, add a `fun_ini_read_name()` helper function that opens `fun.ini`, scans line-by-line for a line matching `name = <value>`, copies the trimmed value into a caller-supplied output buffer, and returns 1 on success or 0 on failure/absent
- [x] 2.2 In `build_generate_windows()`, replace the hardcoded `build/app-windows-amd64.exe` output path with a dynamically built path using `fun_ini_read_name()` (e.g. `build/fun-windows-amd64.exe` when `name = fun`), falling back to `app` if absent
- [x] 2.3 In `build_generate_linux()`, apply the same dynamic name logic for `build/<name>-linux-amd64`

## 3. Comprehensive Vendor Modules in Generated Scripts

- [x] 3.1 In `build_generate_windows()`, extend the vendor module string to include platform, async, process, file (all Windows arch variants: fileRead, fileReadMmap, fileReadRing, fileWrite, fileWriteMmap, fileWriteRing), filesystem (directory, file_exists, path — both src and arch), and array — matching the full set from the hand-maintained `build-windows-amd64.bat`
- [x] 3.2 In `build_generate_linux()`, extend the vendor module string equivalently for Linux arch (process, file, filesystem, array, platform)
- [x] 3.3 Add `-mno-stack-arg-probe` flag to the Windows GCC command in the generated script (present in `build-windows-amd64.bat` but missing from the generator)

## 4. Project Configuration

- [x] 4.1 Create `fun.ini` in the repo root with `name = fun`, `version = 0.1.0`, `description = CLI tool for building fundamental library projects`, and appropriate `[build]` flags

## 5. Build and Verify

- [x] 5.1 Rebuild `fun.exe` with `build-windows-amd64.bat` (bootstrap)
- [x] 5.2 Delete the generated `build-windows-amd64.bat` from the smoke test directory and verify `fun build` regenerates it with the correct binary name (`build/fun-windows-amd64.exe`), real source list, and comprehensive vendor modules
- [x] 5.3 Verify `fun build --clean` on a project with deeply nested `src/` subdirectories discovers all `.c` files correctly
- [x] 5.4 Run the smoke test (`smoke-test-windows-amd64.bat`) to confirm 19/19 pass (smoke test uses `fun` as project name in its generated project — update smoke test expected binary name check if needed)
