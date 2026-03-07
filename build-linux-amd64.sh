#!/bin/bash

# Compile fundamental CLI
gcc \
    --std=c17 -Os \
    -I . \
    -I vendor/fundamental/include \
    src/main.c \
    src/cli.c \
    commands/cmd_version.c \
    commands/cmd_help.c \
    vendor/fundamental/src/console/console.c \
    vendor/fundamental/src/string/stringConversion.c \
    vendor/fundamental/src/string/stringOperations.c \
    vendor/fundamental/src/string/stringTemplate.c \
    vendor/fundamental/src/string/stringValidation.c \
    vendor/fundamental/arch/console/linux-amd64/console.c \
    vendor/fundamental/arch/memory/linux-amd64/memory.c \
    -o cli

echo "Build complete: cli"
