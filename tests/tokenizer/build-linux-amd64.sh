#!/bin/bash
set -e

# Build tokenizer unit tests — Linux AMD64
# Run from: tests/tokenizer/

VENDOR="../../vendor/fundamental"
SRC="../../src"

gcc \
    --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test_tokenizer.c \
    "$SRC/tokenizer/tokenizer.c" \
    "$VENDOR/src/array/array.c" \
    "$VENDOR/src/async/async.c" \
    "$VENDOR/arch/async/linux-amd64/async.c" \
    "$VENDOR/arch/file/linux-amd64/fileRead.c" \
    "$VENDOR/arch/file/linux-amd64/fileReadMmap.c" \
    "$VENDOR/arch/file/linux-amd64/fileReadRing.c" \
    "$VENDOR/arch/file/linux-amd64/fileWrite.c" \
    "$VENDOR/arch/file/linux-amd64/fileWriteMmap.c" \
    "$VENDOR/arch/file/linux-amd64/fileWriteRing.c" \
    "$VENDOR/arch/memory/linux-amd64/memory.c" \
    "$VENDOR/src/string/stringOperations.c" \
    "$VENDOR/src/string/stringConversion.c" \
    "$VENDOR/src/string/stringTemplate.c" \
    -o test

strip --strip-unneeded test
echo "Build complete: test"
