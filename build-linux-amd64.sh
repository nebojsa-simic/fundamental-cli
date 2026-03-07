#!/bin/bash

# Compile fundamental CLI - Complete standalone build
# Uses -nostdlib to exclude standard C library (works on Linux)
# Uses -fno-builtin to prevent compiler builtin substitution
# Uses -e main to specify entry point (required with -nostdlib)
# Our code calls ZERO stdlib runtime functions (printf, malloc, strlen, etc.)

gcc \
    --std=c17 -Os \
    -nostdlib \
    -fno-builtin \
    -e main \
    -I . \
    -I vendor/fundamental/include \
    vendor/fundamental/src/startup/startup.c \
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
    -o fun

echo "Build complete: fun"
