@ECHO OFF

REM Compile fundamental CLI - Zero stdlib function calls
REM Uses -fno-builtin to prevent compiler from substituting our functions
REM with stdlib versions (printf, strlen, etc.)
REM Our code calls ZERO stdlib runtime functions

gcc ^
    --std=c17 -Os ^
    -fno-builtin ^
    -I . ^
    -I vendor/fundamental/include ^
    vendor/fundamental/src/startup/startup.c ^
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
