@ECHO OFF

REM Compile fundamental CLI - Complete standalone build
REM Uses -nostdlib to exclude standard C library
REM Uses -fno-builtin to prevent compiler builtin substitution
REM Uses -e main to specify entry point (required with -nostdlib)
REM Our code calls ZERO stdlib runtime functions (printf, malloc, strlen, etc.)
REM Windows uses custom startup code to parse command line without MinGW runtime

gcc ^
    --std=c17 -Os ^
    -nostdlib ^
    -fno-builtin ^
    -fno-exceptions ^
    -fno-unwind-tables ^
    -e main ^
    -mconsole ^
    -I . ^
    -I vendor/fundamental/include ^
    vendor/fundamental/src/startup/startup.c ^
    vendor/fundamental/src/startup/windows.c ^
    src/main.c ^
    src/cli.c ^
    commands/cmd_version.c ^
    commands/cmd_help.c ^
    vendor/fundamental/src/console/console.c ^
    vendor/fundamental/src/string/stringConversion.c ^
    vendor/fundamental/src/string/stringOperations.c ^
    vendor/fundamental/src/string/stringTemplate.c ^
    vendor/fundamental/src/string/stringValidation.c ^
    vendor/fundamental/arch/console/windows-amd64/console.c ^
    vendor/fundamental/arch/memory/windows-amd64/memory.c ^
    -lkernel32 ^
    -o fun.exe

REM Strip unnecessary symbols
strip --strip-unneeded fun.exe

echo Build complete: fun.exe
