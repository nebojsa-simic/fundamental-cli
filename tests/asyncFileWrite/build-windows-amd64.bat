@ECHO OFF

REM Build async file write test
REM This test reproduces the bug where fun_write_memory_to_file never completes

gcc ^
    --std=c17 -Os ^
    -nostdlib ^
    -fno-builtin ^
    -fno-exceptions ^
    -fno-unwind-tables ^
    -mno-stack-arg-probe ^
    -e main ^
    -mconsole ^
    -mno-stack-arg-probe ^
    -I ..\..\vendor\fundamental\include ^
    ..\..\vendor\fundamental\src\startup\startup.c ^
    ..\..\vendor\fundamental\arch\startup\windows-amd64\windows.c ^
    test.c ^
    ..\..\chkstk_stub.s ^
    ..\..\vendor\fundamental\src\console\console.c ^
    ..\..\vendor\fundamental\src\string\stringConversion.c ^
    ..\..\vendor\fundamental\src\string\stringOperations.c ^
    ..\..\vendor\fundamental\src\string\stringTemplate.c ^
    ..\..\vendor\fundamental\src\string\stringValidation.c ^
    ..\..\vendor\fundamental\arch\memory\windows-amd64\memory.c ^
    ..\..\vendor\fundamental\src\async\async.c ^
    ..\..\vendor\fundamental\arch\file\windows-amd64\fileWrite.c ^
    ..\..\vendor\fundamental\arch\file\windows-amd64\fileWriteMmap.c ^
    ..\..\vendor\fundamental\arch\console\windows-amd64\console.c ^
    -lkernel32 ^
    -o test.exe

if errorlevel 1 (
    echo Build failed
    exit /b 1
)

strip --strip-unneeded test.exe

echo Build complete: test.exe
echo.
echo Running test...
echo.
test.exe

if errorlevel 1 (
    echo.
    echo Test FAILED - async file write hung or errored
    exit /b 1
)

echo.
echo Test PASSED
del test_output.txt 2>nul
