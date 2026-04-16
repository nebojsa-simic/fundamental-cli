#!/bin/bash
set -e

# Build lexer unit tests — Linux AMD64
# Run from: tests/lexer/

VENDOR="../../vendor/fundamental"
SRC="../../src"

echo "Building lexer tests..."

# Common source files for all test binaries
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

# 1b.12: Keyword tests
echo "  Building test_keywords..."
gcc --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test_keywords.c \
    $COMMON_SOURCES \
    -o test_keywords

# 1b.13: Numeric literal tests
echo "  Building test_numeric_literals..."
gcc --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test_numeric_literals.c \
    $COMMON_SOURCES \
    -o test_numeric_literals

# 1b.14: Escape sequence tests
echo "  Building test_escape_sequences..."
gcc --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test_escape_sequences.c \
    $COMMON_SOURCES \
    -o test_escape_sequences

# 1b.15: Pipeline tests
echo "  Building test_pipeline..."
gcc --std=c17 -Os \
    -I "$SRC" \
    -I "$VENDOR/include" \
    test_pipeline.c \
    $COMMON_SOURCES \
    -o test_pipeline

strip --strip-unneeded test_keywords test_numeric_literals test_escape_sequences test_pipeline
echo "Build complete: test_keywords test_numeric_literals test_escape_sequences test_pipeline"
