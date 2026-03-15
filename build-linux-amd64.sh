#!/bin/bash
# Compile fundamental CLI - Complete standalone build
# Uses -nostdlib to exclude standard C library
# Uses -fno-builtin to prevent compiler builtin substitution
# Uses -e main to specify entry point (required with -nostdlib)

gcc \
    --std=c17 -Os \
    -nostdlib \
    -fno-builtin \
    -fno-exceptions \
    -fno-unwind-tables \
    -e main \
    -I . \
    -I vendor/fundamental/include \
    vendor/fundamental/src/startup/startup.c \
    vendor/fundamental/arch/startup/linux-amd64/linux.c \
    src/main.c \
    src/cli.c \
    src/build/detector.c \
    src/build/generator.c \
    src/build/executor.c \
    src/commands/cmd_version.c \
    src/commands/cmd_help.c \
    src/commands/cmd_init.c \
    src/commands/cmd_clean.c \
    src/commands/cmd_build.c \
    src/commands/cmd_test.c \
    src/commands/cmd_test_add.c \
    src/fun/platform.c \
    src/test/discovery.c \
    src/test/scaffolder.c \
    src/test/module_map.c \
    src/test/runner.c \
    src/test/reporter.c \
    vendor/fundamental/src/platform/platform.c \
    vendor/fundamental/arch/platform/linux-amd64/platform.c \
    vendor/fundamental/src/async/async.c \
    vendor/fundamental/src/process/process.c \
    vendor/fundamental/arch/process/linux-amd64/process.c \
    vendor/fundamental/arch/file/linux-amd64/fileRead.c \
    vendor/fundamental/arch/file/linux-amd64/fileReadMmap.c \
    vendor/fundamental/arch/file/linux-amd64/fileReadRing.c \
    vendor/fundamental/arch/file/linux-amd64/fileWrite.c \
    vendor/fundamental/arch/file/linux-amd64/fileWriteMmap.c \
    vendor/fundamental/arch/file/linux-amd64/fileWriteRing.c \
    vendor/fundamental/src/console/console.c \
    vendor/fundamental/src/string/stringConversion.c \
    vendor/fundamental/src/string/stringOperations.c \
    vendor/fundamental/src/string/stringTemplate.c \
    vendor/fundamental/src/string/stringValidation.c \
    vendor/fundamental/src/array/array.c \
    vendor/fundamental/arch/console/linux-amd64/console.c \
    vendor/fundamental/arch/memory/linux-amd64/memory.c \
    vendor/fundamental/src/filesystem/directory.c \
    vendor/fundamental/src/filesystem/file_exists.c \
    vendor/fundamental/src/filesystem/path.c \
    vendor/fundamental/arch/filesystem/linux-amd64/directory.c \
    vendor/fundamental/arch/filesystem/linux-amd64/file_exists.c \
    vendor/fundamental/arch/filesystem/linux-amd64/path.c \
    -o fun

strip --strip-unneeded fun
echo "Build complete: fun"
