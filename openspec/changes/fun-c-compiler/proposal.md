## Why

The `fun build` command currently generates a shell/batch script that invokes GCC for compilation. This means every user needs GCC installed, and the toolchain is not self-contained. A built-in C compiler (funcc) would make the fundamental ecosystem fully self-hosted: fun compiles C code directly, with no external compiler dependency.

This also serves as a proof-of-concept that the fundamental library is capable of building non-trivial systems software — a compiler is one of the most demanding applications for a zero-stdlib C library.

## What Changes

- **New**: C compiler (`funcc`) implemented as a `fun compile` command
- **New**: Preprocessor — `#include`, `#define`, `#ifdef`, `#pragma`, `##` token pasting, `__VA_ARGS__`
- **New**: Lexer — tokenizes preprocessed C source
- **New**: Parser — produces AST from token stream
- **New**: Type system — semantic analysis, name resolution, type checking
- **New**: Code generator — emits x86-64 assembly (AT&T syntax)
- **Modified**: `fun build` gains option to use funcc instead of GCC
- **External dependency**: assembler (nasm or as) and linker (ld or link.exe) still required

## Scope

### C Subset (sufficient for fundamental-cli + fundamental library)

**Types**: `int`, `char`, `void`, `float`, `double`, stdint types (`uint8_t`..`uint64_t`, `size_t`, `uintptr_t`), `struct`, `union`, `enum`, `typedef`, function pointers

**Declarations**: `static`, `const`, `static inline`, compound literals with designated initializers

**Control flow**: `if`/`else`, `for`, `while`, `do`/`while`, `switch`/`case`, `break`, `continue`, `return`

**Operators**: all arithmetic, bitwise, logical, ternary, `sizeof`, casts, `&`, `*`, `->`, `.`

**Preprocessor**: `#include` (quoted and angle-bracket), `#define` (object-like and function-like), `#ifdef`/`#ifndef`/`#if`/`#endif`, `#pragma once`, `#pragma GCC diagnostic`, `##` token pasting, `__VA_ARGS__`

### Explicitly Not In Scope

- `_Generic`, `_Alignas`, VLA, `typeof`, `__attribute__`
- `goto`
- Variadic functions (`va_list`, `va_arg`)
- Flexible array members
- Full C17 conformance (targeting the subset actually used)
- Optimization passes (emit correct code, not fast code)
- Built-in assembler or linker

## Capabilities

### New Capabilities

- `c-preprocessor`: Expand includes, macros, conditionals into a single translation unit
- `c-lexer`: Tokenize preprocessed C source into a typed token stream
- `c-parser`: Parse tokens into an abstract syntax tree
- `c-type-system`: Semantic analysis — type checking, name resolution, scope management
- `c-codegen`: Emit x86-64 assembly from typed AST (Win64 and System V ABIs)
- `fun-compile-command`: `fun compile <file.c>` command to compile a single translation unit

### Modified Capabilities

- `fun-build`: Option to use funcc instead of GCC for compilation

## Impact

- **Build system**: `fun build` can optionally bypass GCC entirely (except assembler + linker)
- **User requirements**: Users no longer need GCC installed (only assembler + linker)
- **Bootstrap**: funcc is initially compiled with GCC; self-hosting is a future milestone
- **Binary size**: The fun binary will grow significantly (compiler is substantial code)
- **Fundamental library**: May need additions to string module (char classification, substring extraction)
