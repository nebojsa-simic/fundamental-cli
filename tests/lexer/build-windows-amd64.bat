@ECHO OFF
REM Build lexer unit tests — Windows AMD64
REM Run from: tests/lexer/

set VENDOR=..\..\vendor\fundamental
set SRC=..\..\src

echo Building lexer tests...

REM 1b.12: Keyword tests
echo   Building test_keywords...
gcc ^
    --std=c17 -Os ^
    -I %SRC% ^
    -I %VENDOR%\include ^
    test_keywords.c ^
    %SRC%\tokenizer\tokenizer.c ^
    %SRC%\tokenizer\lexer.c ^
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
    -lkernel32 ^
    -o test_keywords.exe

IF %ERRORLEVEL% NEQ 0 (
    echo Build FAILED: test_keywords
    exit /b 1
)

REM 1b.13: Numeric literal tests
echo   Building test_numeric_literals...
gcc ^
    --std=c17 -Os ^
    -I %SRC% ^
    -I %VENDOR%\include ^
    test_numeric_literals.c ^
    %SRC%\tokenizer\tokenizer.c ^
    %SRC%\tokenizer\lexer.c ^
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
    -lkernel32 ^
    -o test_numeric_literals.exe

IF %ERRORLEVEL% NEQ 0 (
    echo Build FAILED: test_numeric_literals
    exit /b 1
)

REM 1b.14: Escape sequence tests
echo   Building test_escape_sequences...
gcc ^
    --std=c17 -Os ^
    -I %SRC% ^
    -I %VENDOR%\include ^
    test_escape_sequences.c ^
    %SRC%\tokenizer\tokenizer.c ^
    %SRC%\tokenizer\lexer.c ^
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
    -lkernel32 ^
    -o test_escape_sequences.exe

IF %ERRORLEVEL% NEQ 0 (
    echo Build FAILED: test_escape_sequences
    exit /b 1
)

REM 1b.15: Pipeline tests
echo   Building test_pipeline...
gcc ^
    --std=c17 -Os ^
    -I %SRC% ^
    -I %VENDOR%\include ^
    test_pipeline.c ^
    %SRC%\tokenizer\tokenizer.c ^
    %SRC%\tokenizer\lexer.c ^
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
    -lkernel32 ^
    -o test_pipeline.exe

IF %ERRORLEVEL% NEQ 0 (
    echo Build FAILED: test_pipeline
    exit /b 1
)

strip --strip-unneeded test_keywords.exe test_numeric_literals.exe test_escape_sequences.exe test_pipeline.exe
echo Build complete: test_keywords.exe test_numeric_literals.exe test_escape_sequences.exe test_pipeline.exe
