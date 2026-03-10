#!/bin/bash
# Compile fun CLI
gcc --std=c17 -Os -nostdlib -fno-builtin -fno-exceptions -fno-unwind-tables -e main -I . -I vendor/fundamental/include vendor/fundamental/src/startup/startup.c vendor/fundamental/arch/startup/linux-amd64/linux.c src/main.c src/cli.c src/commands/cmd_version.c src/commands/cmd_help.c vendor/fundamental/src/console/console.c vendor/fundamental/src/string/stringConversion.c vendor/fundamental/src/string/stringOperations.c vendor/fundamental/src/string/stringTemplate.c vendor/fundamental/src/string/stringValidation.c vendor/fundamental/arch/console/linux-amd64/console.c vendor/fundamental/arch/memory/linux-amd64/memory.c -o fun
strip --strip-unneeded fun
echo Build complete: fun
sion.c \
    vendor/fundamental/src/string/stringOperations.c \
    vendor/fundamental/src/string/stringTemplate.c \
    vendor/fundamental/src/string/stringValidation.c \
    vendor/fundamental/arch/console/linux-amd64/console.c \
    vendor/fundamental/arch/memory/linux-amd64/memory.c \
    -o fun

echo "Build complete: fun"
