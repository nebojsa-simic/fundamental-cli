@ECHO OFF

REM Build tokenizer unit tests — Windows AMD64
REM Run from: tests/tokenizer/

set VENDOR=..\..\vendor\fundamental
set SRC=..\..\src

gcc ^
    --std=c17 -Os ^
    -fno-exceptions -fno-unwind-tables ^
    -mno-stack-arg-probe -mconsole ^
    -I %SRC% ^
    -I %VENDOR%\include ^
    test_tokenizer.c ^
    %SRC%\tokenizer\tokenizer.c ^
    %VENDOR%\src\array\array.c ^
    %VENDOR%\src\async\async.c ^
    %VENDOR%\arch\async\windows-amd64\async.c ^
    %VENDOR%\arch\file\windows-amd64\fileRead.c ^
    %VENDOR%\arch\file\windows-amd64\fileReadMmap.c ^
    %VENDOR%\arch\file\windows-amd64\fileReadRing.c ^
    %VENDOR%\arch\file\windows-amd64\fileWrite.c ^
    %VENDOR%\arch\file\windows-amd64\fileWriteMmap.c ^
    %VENDOR%\arch\file\windows-amd64\fileWriteRing.c ^
    %VENDOR%\arch\memory\windows-amd64\memory.c ^
    %VENDOR%\src\string\stringOperations.c ^
    %VENDOR%\src\string\stringConversion.c ^
    %VENDOR%\src\string\stringTemplate.c ^
    %VENDOR%\src\console\console.c ^
    %VENDOR%\arch\console\windows-amd64\console.c ^
    %VENDOR%\src\filesystem\path.c ^
    %VENDOR%\src\filesystem\file_exists.c ^
    %VENDOR%\src\filesystem\file_size.c ^
    %VENDOR%\arch\filesystem\windows-amd64\path.c ^
    %VENDOR%\arch\filesystem\windows-amd64\file_exists.c ^
    %VENDOR%\arch\filesystem\windows-amd64\file_size.c ^
    %VENDOR%\arch\filesystem\windows-amd64\directory.c ^
    %VENDOR%\src\platform\platform.c ^
    %VENDOR%\arch\platform\windows-amd64\platform.c ^
    -lkernel32 ^
    -o test.exe

IF %ERRORLEVEL% NEQ 0 (
    ECHO Build FAILED
    exit /b 1
)

strip --strip-unneeded test.exe
ECHO Build complete: test.exe
