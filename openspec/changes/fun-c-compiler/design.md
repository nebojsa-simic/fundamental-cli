## Context

The `fun build` command generates a platform-specific script that calls GCC with hardcoded flags and source file lists. The compiler project replaces GCC with an in-process C compiler (funcc) that handles preprocessing through code generation, emitting x86-64 assembly for an external assembler and linker.

The compiler is written using the fundamental library (zero-stdlib) and targets the C subset actually used by fundamental-cli and the fundamental library itself.

## Goals / Non-Goals

**Goals:**
- Compile the C subset used by fundamental-cli and fundamental library
- Emit correct x86-64 assembly for both Windows (Win64 ABI) and Linux (System V ABI)
- Full preprocessor including macros, token pasting, and conditional compilation
- Clear error messages with file, line, and column information
- Bootstrapped with GCC (funcc compiled by GCC initially)

**Non-Goals:**
- Full C17 conformance
- Optimization passes (correct code, not fast code)
- Built-in assembler or linker
- Self-hosting (future milestone, not part of this change)
- Debug info emission (DWARF, PDB)

## Architecture

```
                         fun compile <file.c>
                                │
                                ▼
                    ┌───────────────────────┐
                    │     Preprocessor      │
                    │                       │
                    │  #include resolution  │
                    │  #define expansion    │
                    │  #ifdef evaluation   │
                    │  ## token pasting     │
                    │  __VA_ARGS__          │
                    └───────────┬───────────┘
                                │ expanded source text
                                ▼
                    ┌───────────────────────┐
                    │        Lexer          │
                    │                       │
                    │  Characters → Tokens  │
                    │  (keyword, ident,     │
                    │   literal, operator,  │
                    │   punctuation)        │
                    └───────────┬───────────┘
                                │ Array<Token>
                                ▼
                    ┌───────────────────────┐
                    │       Parser          │
                    │                       │
                    │  Tokens → AST         │
                    │  (recursive descent)  │
                    └───────────┬───────────┘
                                │ AST
                                ▼
                    ┌───────────────────────┐
                    │  Semantic Analysis    │
                    │                       │
                    │  Name resolution      │
                    │  Type checking        │
                    │  Scope management     │
                    └───────────┬───────────┘
                                │ Typed AST + symbol tables
                                ▼
                    ┌───────────────────────┐
                    │   Code Generator      │
                    │                       │
                    │  Typed AST → x86-64   │
                    │  assembly (AT&T)      │
                    │                       │
                    │  Win64 or SysV ABI    │
                    └───────────┬───────────┘
                                │ .s file
                                ▼
                    ┌───────────────────────┐
                    │  External Toolchain   │
                    │                       │
                    │  as/nasm → .o         │
                    │  ld/link → binary     │
                    └───────────────────────┘
```

### File Structure

```
src/
  compiler/
    preprocessor.c / .h     Preprocessor (include, define, ifdef, macros)
    lexer.c / .h             Lexer (tokenization)
    token.c / .h             Token types and utilities
    parser.c / .h            Recursive descent parser
    ast.c / .h               AST node types and constructors
    sema.c / .h              Semantic analysis (type checking, scoping)
    type.c / .h              Type representation and operations
    symtab.c / .h            Symbol table (scoped name → type mapping)
    codegen.c / .h           x86-64 assembly emission
    codegen_win64.c / .h     Win64 ABI specifics
    codegen_sysv.c / .h      System V ABI specifics
    compiler.c / .h          Top-level driver (orchestrates all phases)
  commands/
    cmd_compile.c / .h       The `fun compile` command
```

## Decisions

### Decision 1: Recursive descent parser (not table-driven)

**Rationale:** Recursive descent maps directly to the grammar, is easy to debug, produces good error messages, and handles C's declaration syntax naturally. Every major production C compiler (GCC, Clang, MSVC, tcc) uses recursive descent for parsing.

**Alternatives considered:**
- LR/LALR parser (yacc): C's grammar is ambiguous (typedef-name vs identifier), requires hacks
- PEG parser: Unusual for C, harder to produce good errors

### Decision 2: Emit AT&T syntax x86-64 assembly

**Rationale:** AT&T syntax is the default for GNU `as` (available on both platforms via MinGW on Windows). Single assembly syntax simplifies the code generator.

**Alternatives considered:**
- Intel syntax (NASM): Would need NASM installed, less common in MinGW toolchains
- Direct machine code emission: Vastly more complex, essentially building an assembler
- LLVM IR: External dependency on LLVM, defeats self-contained goal

### Decision 3: Separate ABI modules for Win64 and System V

**Rationale:** Calling conventions differ significantly (register assignments, stack alignment, shadow space vs red zone). Isolating ABI details prevents the code generator from becoming a tangle of platform conditionals.

**Alternatives considered:**
- Single codegen with #ifdef: Messy, error-prone for testing
- Abstract ABI interface: Over-engineering for two targets

### Decision 4: Build phases in recommended order (lexer first, not preprocessor)

**Rationale:** The lexer is immediately testable on hand-written input, produces visible results fast, and forces early definition of the Token type that all later phases depend on. The preprocessor is complex and can be deferred — during development, GCC's `-E` flag can provide preprocessed input for testing.

**Alternatives considered:**
- Preprocessor first: Theoretically cleaner pipeline, but weeks of work before any visible progress
- All phases in parallel: Too many interdependencies

### Decision 5: Fundamental library additions handled in fundamental repo

**Rationale:** Character classification (`is_digit`, `is_alpha`), substring extraction, and similar utilities belong in the fundamental string module, not as compiler-specific code.

**Additions needed in fundamental:**
- `fun_string_char_is_digit(char c)` — and similar classification functions
- `fun_string_substring(source, start, length, output, output_size)` — extract substring
- These are vendored into fundamental-cli once available

**Workaround until available:** `DEFINE_ARRAY_TYPE(char)` for growable buffers; inline char classification is trivial.

## Data Structures

### Token

```c
typedef enum {
    // Keywords
    TOK_INT, TOK_CHAR, TOK_VOID, TOK_FLOAT, TOK_DOUBLE,
    TOK_STRUCT, TOK_UNION, TOK_ENUM, TOK_TYPEDEF,
    TOK_IF, TOK_ELSE, TOK_FOR, TOK_WHILE, TOK_DO,
    TOK_SWITCH, TOK_CASE, TOK_DEFAULT,
    TOK_BREAK, TOK_CONTINUE, TOK_RETURN,
    TOK_STATIC, TOK_CONST, TOK_INLINE, TOK_EXTERN,
    TOK_SIZEOF,

    // Stdint keywords (resolved via typedef, but recognized after preprocessing)
    TOK_IDENTIFIER,

    // Literals
    TOK_INT_LITERAL,      // 42, 0x2A, 052
    TOK_FLOAT_LITERAL,    // 3.14, 1.0f
    TOK_CHAR_LITERAL,     // 'a', '\n'
    TOK_STRING_LITERAL,   // "hello"

    // Operators (single char)
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_AMPERSAND, TOK_PIPE, TOK_CARET, TOK_TILDE,
    TOK_BANG, TOK_ASSIGN, TOK_LT, TOK_GT,
    TOK_DOT, TOK_QUESTION, TOK_COLON,

    // Operators (multi char)
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN, TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN, TOK_PERCENT_ASSIGN,
    TOK_AMP_ASSIGN, TOK_PIPE_ASSIGN, TOK_CARET_ASSIGN,
    TOK_LSHIFT_ASSIGN, TOK_RSHIFT_ASSIGN,
    TOK_EQ, TOK_NEQ, TOK_LTE, TOK_GTE,
    TOK_AND, TOK_OR,
    TOK_LSHIFT, TOK_RSHIFT,
    TOK_INCREMENT, TOK_DECREMENT,
    TOK_ARROW,

    // Punctuation
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_SEMICOLON, TOK_COMMA,
    TOK_ELLIPSIS,   // ...

    // Special
    TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    String    value;       // points into source buffer
    uint32_t  value_len;   // length of value (avoids null-terminator requirement)
    uint32_t  line;
    uint32_t  col;
} Token;
```

### AST Node Types (~30)

```c
typedef enum {
    // Top-level
    AST_TRANSLATION_UNIT,
    AST_FUNCTION_DEF,
    AST_VAR_DECL,
    AST_STRUCT_DEF,
    AST_UNION_DEF,
    AST_ENUM_DEF,
    AST_TYPEDEF,

    // Statements
    AST_BLOCK,
    AST_IF_STMT,
    AST_FOR_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_EXPR_STMT,

    // Expressions
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_TERNARY_EXPR,
    AST_CAST_EXPR,
    AST_SIZEOF_EXPR,
    AST_CALL_EXPR,
    AST_MEMBER_EXPR,
    AST_INDEX_EXPR,
    AST_IDENT_EXPR,
    AST_LITERAL_EXPR,
    AST_COMPOUND_LITERAL,
    AST_DESIGNATED_INIT,
} ASTNodeType;
```

### Symbol Table

```
Per-scope HashMap<String, Symbol> stacked in an array:

    ┌─────────────────────┐
    │ Global scope        │  typedefs, functions, global vars
    ├─────────────────────┤
    │ Function scope      │  parameters, local vars
    ├─────────────────────┤
    │ Block scope         │  block-local vars
    ├─────────────────────┤
    │ Block scope         │  nested block vars
    └─────────────────────┘

    Lookup walks from top of stack to bottom.
    Push on '{', pop on '}'.
```

### Type Representation

```c
typedef enum {
    TYPE_VOID, TYPE_CHAR, TYPE_INT, TYPE_FLOAT, TYPE_DOUBLE,
    TYPE_INT8, TYPE_UINT8, TYPE_INT16, TYPE_UINT16,
    TYPE_INT32, TYPE_UINT32, TYPE_INT64, TYPE_UINT64,
    TYPE_SIZE_T, TYPE_UINTPTR,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_FUNCTION,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    union {
        struct { struct Type *pointee; } pointer;
        struct { struct Type *element; uint64_t size; } array;
        struct { String name; /* field list */ } record;   // struct or union
        struct { String name; /* enumerator list */ } enumeration;
        struct { struct Type *return_type; /* param list */ } function;
    };
    int is_const;
} Type;
```

## Risks / Trade-offs

**[C declaration syntax is notoriously hard to parse]** → Mitigation: Recursive descent with a "declaration specifiers + declarator" model (the standard approach). The subset excludes the worst cases (no K&R style, no complex function pointer nesting beyond what fundamental uses).

**[Preprocessor macro expansion is a compiler in itself]** → Mitigation: Target only the macros actually used (DEFINE_RESULT_TYPE, DEFINE_ARRAY_TYPE, DEFINE_HASHMAP_TYPE, etc.). These use `##` and `__VA_ARGS__` but are structurally regular.

**[Two ABIs (Win64 and System V) doubles codegen testing]** → Mitigation: Separate ABI modules with shared core. Test on the native platform first, cross-platform second.

**[Binary size increase]** → Mitigation: The compiler adds significant code to `fun.exe`. Acceptable trade-off for self-contained toolchain. Can be made optional (compile out if not needed).

**[No optimization = slow generated code]** → Mitigation: Correct code first. Optimization is a separate future change. Users who need performance can still use GCC.

## Phased Implementation Order

```
Phase 1: Lexer + Token types
         → testable immediately with hand-written input

Phase 2: Parser (expressions only)
         → testable with expression strings

Phase 3: Preprocessor (#include + simple #define)
         → can now lex real files (use gcc -E to validate)

Phase 4: Parser (declarations + statements)
         → can parse full .c files

Phase 5: Preprocessor (function macros, ##, __VA_ARGS__, #ifdef)
         → can preprocess real files without gcc -E

Phase 6: Type system + semantic analysis
         → can type-check fundamental-cli source files

Phase 7: Code generator (expressions + functions)
         → can compile minimal programs (main returning 42)

Phase 8: Code generator (control flow + structs)
         → can compile non-trivial programs

Phase 9: Integration with fun build
         → fun build --compiler=funcc as alternative to gcc

Phase 10: Validation
          → compile fundamental-cli with funcc, compare output
```

## Open Questions

- Should funcc be a separate binary or a `fun compile` subcommand? (Current design: subcommand)
- Should the preprocessor output be cacheable for incremental builds?
- What assembler to target — GNU `as` (available in MinGW), or NASM?
- Should funcc support `-E` (preprocess only) and `-S` (compile to asm only) flags for debugging?
- When should self-hosting become a goal?
