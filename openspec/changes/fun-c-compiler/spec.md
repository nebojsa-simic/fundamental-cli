## C Subset Specification — funcc

This documents the exact C language subset that funcc targets, derived from analysis of the fundamental-cli and fundamental library codebases.

### Types

```gherkin
Feature: Primitive types
  Supported: int, char, void, float, double

Feature: Fixed-width integer types (via stdint.h typedefs)
  Supported: int8_t, uint8_t, int16_t, uint16_t,
             int32_t, uint32_t, int64_t, uint64_t,
             size_t, uintptr_t

Feature: Aggregate types
  Supported: struct, union, enum

Feature: Type qualifiers
  Supported: const
  Not supported: volatile, restrict, _Atomic

Feature: Storage classes
  Supported: static, extern, inline (static inline)
  Not supported: register, _Thread_local

Feature: Derived types
  Supported: pointers (including pointer-to-pointer),
             fixed-size arrays, function pointers
  Not supported: VLA, flexible array members

Feature: typedef
  Supported: simple typedefs, typedef for function pointers,
             typedef for struct/union/enum
```

### Declarations

```gherkin
Feature: Variable declarations
  Supported: with and without initializer,
             designated initializers for structs,
             compound literals: (Type){ .field = value }

Feature: Function declarations
  Supported: prototypes in headers, definitions with body,
             static inline in headers

Feature: Struct/union definitions
  Supported: named structs/unions with field list,
             nested struct member access,
             anonymous struct members NOT required

Feature: Enum definitions
  Supported: named enums with explicit or implicit values

Feature: Forward declarations
  Supported: struct/union forward declarations (opaque pointers)
```

### Expressions

```gherkin
Feature: Arithmetic operators
  Supported: +, -, *, /, %

Feature: Bitwise operators
  Supported: &, |, ^, ~, <<, >>

Feature: Logical operators
  Supported: &&, ||, !

Feature: Comparison operators
  Supported: ==, !=, <, >, <=, >=

Feature: Assignment operators
  Supported: =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=

Feature: Unary operators
  Supported: -, !, ~, &, *, ++, -- (prefix and postfix)

Feature: Other operators
  Supported: sizeof, cast, ternary (?:), comma,
             function call, array index, member access (. ->)
```

### Statements

```gherkin
Feature: Control flow
  Supported: if/else, for, while, do-while,
             switch/case/default,
             break, continue, return
  Not supported: goto, labels (except case/default)

Feature: Compound statements
  Supported: { } blocks with mixed declarations and statements (C99+)
```

### Preprocessor

```gherkin
Feature: File inclusion
  Supported: #include "file.h" (relative path search)
             #include <file.h> (system path search)

Feature: Object-like macros
  Supported: #define NAME value
             #define NAME (no value, for guards)
             #undef NAME

Feature: Function-like macros
  Supported: #define FOO(a, b) expression
             ## token pasting (e.g., T##Result)
             __VA_ARGS__ in variadic macros
             # stringification operator

Feature: Conditional compilation
  Supported: #ifdef, #ifndef, #if, #elif, #else, #endif
             defined() operator in #if expressions
             Nested conditionals

Feature: Pragmas
  Supported: #pragma once
             #pragma GCC diagnostic push/pop/ignored (ignored or passed through)

Feature: Predefined macros
  Supported: __FILE__, __LINE__, NULL

Feature: Include guards
  Supported: #ifndef GUARD / #define GUARD / #endif pattern
             #pragma once (equivalent behavior)
```

### Features Explicitly NOT Supported

```
_Generic
_Alignas / _Alignof
_Static_assert
_Noreturn
VLA (variable length arrays)
typeof / __typeof__
__attribute__((...))
goto / labels
Variadic functions (va_list, va_arg, va_start, va_end)
Flexible array members
K&R style function definitions
Complex declarators beyond what fundamental uses
Bit-fields in structs
Designated initializers for arrays (only structs)
Inline assembly (asm / __asm__)
_Complex / _Imaginary
```
