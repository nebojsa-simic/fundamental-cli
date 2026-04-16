#!/bin/bash
# Compile fundamental CLI - Complete standalone build
# Uses -nostdlib to exclude standard C library
# Uses -fno-builtin to prevent compiler builtin substitution
# Uses -e main to specify entry point (required with -nostdlib)

mkdir -p build

gcc \
    --std=c17 -Os \
    -nostdlib \
    -fno-builtin \
    -fno-exceptions \
    -fno-unwind-tables \
    -I . \
    -I include \
    -I src \
    -I vendor/fundamental/include \
    vendor/fundamental/src/startup/startup.c \
    vendor/fundamental/arch/startup/linux-amd64/linux.c \
    src/main.c \
    src/cli/cli.c \
    src/build/config.c \
    src/build/detector.c \
    arch/build/linux-amd64/detector.c \
    src/build/executor.c \
    arch/build/linux-amd64/executor.c \
    src/build/generator.c \
    arch/build/linux-amd64/generator.c \
    arch/build/linux-amd64/platform.c \
    src/commands/cmd_version.c \
    src/commands/cmd_help.c \
    src/commands/cmd_init.c \
    src/commands/cmd_clean.c \
    src/commands/cmd_build.c \
    src/commands/cmd_test.c \
    src/commands/cmd_test_add.c \
    src/test/discovery.c \
    src/test/scaffolder.c \
    src/test/module_map.c \
    src/test/runner.c \
    src/test/reporter.c \
    src/tokenizer/tokenizer.c \
    src/tokenizer/lexer.c \
    vendor/fundamental/src/platform/platform.c \
    vendor/fundamental/arch/platform/linux-amd64/platform.c \
    vendor/fundamental/src/async/async.c \
    vendor/fundamental/arch/async/linux-amd64/async.c \
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
    vendor/fundamental/src/hashmap/hashmap.c \
    vendor/fundamental/src/config/config.c \
    vendor/fundamental/src/config/iniParser.c \
    vendor/fundamental/src/config/cliParser.c \
    vendor/fundamental/arch/config/linux-amd64/env.c \
    vendor/fundamental/arch/console/linux-amd64/console.c \
    vendor/fundamental/arch/memory/linux-amd64/memory.c \
    vendor/fundamental/src/filesystem/directory.c \
    vendor/fundamental/src/filesystem/file_exists.c \
    vendor/fundamental/src/filesystem/path.c \
    vendor/fundamental/src/filesystem/walk.c \
    vendor/fundamental/src/tsv/tsv.c \
    vendor/fundamental/arch/filesystem/linux-amd64/directory.c \
    vendor/fundamental/arch/filesystem/linux-amd64/file_exists.c \
    vendor/fundamental/arch/filesystem/linux-amd64/path.c \
    -o build/fun-linux-amd64

strip --strip-unneeded build/fun-linux-amd64
echo "Build complete: build/fun-linux-amd64"
