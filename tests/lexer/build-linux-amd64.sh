#!/bin/bash
set -e

# Build lexer unit tests — Linux AMD64
# Run from: tests/lexer/

VENDOR="../../vendor/fundamental"
SRC="../../src"

echo "Building lexer tests..."

COMMON_SOURCES="
../../src/tokenizer/tokenizer.c \
../../src/tokenizer/lexer.c \
../../vendor/fundamental/src/array/array.c \
../../vendor/fundamental/src/async/async.c \
../../vendor/fundamental/arch/async/linux-amd64/async.c \
../../vendor/fundamental/arch/file/linux-amd64/fileRead.c \
../../vendor/fundamental/arch/file/linux-amd64/fileReadMmap.c \
../../vendor/fundamental/arch/file/linux-amd64/fileReadRing.c \
../../vendor/fundamental/arch/file/linux-amd64/fileWrite.c \
../../vendor/fundamental/arch/file/linux-amd64/fileWriteMmap.c \
../../vendor/fundamental/arch/file/linux-amd64/fileWriteRing.c \
../../vendor/fundamental/arch/memory/linux-amd64/memory.c \
../../vendor/fundamental/src/string/stringOperations.c \
../../vendor/fundamental/src/string/stringConversion.c \
../../vendor/fundamental/src/string/stringTemplate.c \
"

gcc --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test.c \
    $COMMON_SOURCES \
    -o test

strip --strip-unneeded test
echo "Build complete: test"
