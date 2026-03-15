## 1. Update Build Script Generator

- [x] 1.1 In `build_generate_windows()` in `src/build/generator.c`, add `if not exist build mkdir build\r\n` before the GCC command string
- [x] 1.2 In `build_generate_windows()`, change the `-o app.exe` output flag to `-o build/app-windows-amd64.exe`
- [x] 1.3 In `build_generate_windows()`, update the `strip` line from `strip --strip-unneeded app.exe` to `strip --strip-unneeded build/app-windows-amd64.exe`
- [x] 1.4 In `build_generate_windows()`, update the `ECHO Build complete:` line to reference `build/app-windows-amd64.exe`
- [x] 1.5 In `build_generate_linux()`, add `mkdir -p build\n` before the GCC command string
- [x] 1.6 In `build_generate_linux()`, change the `-o app` output flag to `-o build/app-linux-amd64`
- [x] 1.7 In `build_generate_linux()`, update the `strip` line from `strip --strip-unneeded app` to `strip --strip-unneeded build/app-linux-amd64`
- [x] 1.8 In `build_generate_linux()`, update the `echo Build complete:` line to reference `build/app-linux-amd64`

## 2. Update Clean Command

- [x] 2.1 In `cmd_clean.c`, replace the `remove_file(binary_name)` call (which targets `app.exe`/`app` in the root) with a `remove_directory("build")` implementation that runs `rmdir /s /q build` (Windows) or `rm -rf build` (Linux) via `fun_process_spawn`
- [x] 2.2 Remove the now-unused `binary_name` variable and `remove_file` call for the root-level binary

## 3. Build and Verify

- [x] 3.1 Rebuild `fun.exe` with `build-windows-amd64.bat`
- [x] 3.2 Delete any pre-existing generated `build-windows-amd64.bat` in a test project and run `fun build` to verify the binary appears in `build/app-windows-amd64.exe`
- [x] 3.3 Run `fun clean` and verify `build/` directory is removed
- [x] 3.4 Run the smoke test (`smoke-test-windows-amd64.bat`) to confirm end-to-end pass
