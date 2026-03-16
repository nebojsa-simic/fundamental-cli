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

Every stage in the pipeline produces a well-defined intermediate result that
can be serialized to disk, inspected, and resumed from. Each stage is a pure
function: input IR in, output IR out.

```
fun compile <file.c>
│
│  ┌──────────────────────────────────────────────────────────────────┐
│  │                         PIPELINE                                │
│  │                                                                  │
│  │  Stage          Intermediate Result       Disk Format   Flag     │
│  │  ─────          ───────────────────       ───────────   ────     │
▼  │                                                                  │
│  │                  ┄┄ PREPROCESSOR ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄     │
│  │                                                                  │
┌──┤  Normalize      NormalizedSource          .norm (text)  --norm   │
│  │       │          + LineMap                                        │
│  │       ▼                                                          │
│  │  Include        TranslationUnit           .inc (text)   --inc   │
│  │       │          (all includes inlined)                          │
│  │       ▼                                                          │
│  │  Conditional    ConditionalResult         .cond (text)  --cond  │
│  │       │          + MacroTable                                    │
│  │       ▼                                                          │
│  │  Macro Expand   ExpandedSource            .i (text)     -E      │
│  │       │                                                          │
│  │                  ┄┄ FRONT-END ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄     │
│  │       ▼                                                          │
│  │  Tokenizer      Array<RawToken>           .tokens (bin) --tokens │
│  │       │                                                          │
│  │       ▼                                                          │
│  │  Lexer          Array<LexToken>           .lex (bin)    --lex   │
│  │       │           + StringTable                                  │
│  │       ▼                                                          │
│  │  Parser         AST                       .ast (bin)    --ast   │
│  │       │                                                          │
│  │                  ┄┄ SEMANTIC ANALYSIS ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄     │
│  │       ▼                                                          │
│  │  Sym Collect    AST + SymbolTable         .sym (bin)    --symbols│
│  │       │                                                          │
│  │       ▼                                                          │
│  │  Type Resolve   AST + SymbolTable         .types (bin)  --types │
│  │       │           + TypeGraph                                    │
│  │       ▼                                                          │
│  │  Name Resolve   ResolvedAST               .resolved     --resolved
│  │       │           + SymbolTable             (bin)                │
│  │       │           + TypeGraph                                    │
│  │       ▼                                                          │
│  │  Type Check     TypedAST                  .typed (bin)  --typed │
│  │       │           + SymbolTable                                  │
│  │       │           + TypeGraph                                    │
│  │                                                                  │
│  │                  ┄┄ BACK-END ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄     │
│  │       ▼                                                          │
│  │  Code Gen       Assembly                  .s (text)     -S      │
│  │       │                                                          │
│  │       ▼                                                          │
│  │  Assemble       Object code               .o (bin)      -c      │
│  │  (external:     (via GNU as)                                     │
│  │   GNU as)                                                        │
│  │       │                                                          │
│  │       ▼                                                          │
│  │  Link           Executable                .exe / ELF    (default)│
│  │  (external:     (via ld / link)                                  │
│  │   ld/link)                                                       │
│  └──────────────────────────────────────────────────────────────────┘
```

Each flag stops the pipeline at that stage and writes the intermediate result
to disk. Inspect commands provide human-readable views of binary formats:

```
fun inspect-tokens <file.tokens>     human-readable raw token dump
fun inspect-lex    <file.lex>        human-readable classified token dump
fun inspect-ast    <file.ast>        human-readable AST dump
fun inspect-sym    <file.sym>        human-readable symbol table dump
fun inspect-types  <file.types>      human-readable type graph dump
```

Note: preprocessor intermediate results (.norm, .inc, .cond, .i) are all
plain text and human-readable directly — no inspect command needed.

### Preprocessor — Four-Stage Split

The preprocessor is split into four passes. Each produces a text intermediate
result that is human-readable and writable to disk without an inspect command.

```
file.c (raw source)
  │
  ▼
┌───────────────────────────────────────┐
│  3a: Source Normalization             │
│                                       │
│  Pure text → text. Zero semantic      │
│  knowledge. Mechanical cleanup.       │
│                                       │
│  - Join backslash-continued lines     │
│    (line splicing)                    │
│  - Remove // line comments            │
│  - Remove /* block comments */        │
│  - Normalize line endings (CRLF → LF)│
│  - Build line mapping (original line  │
│    numbers for error reporting after  │
│    line splicing changes positions)   │
└──────────────┬────────────────────────┘
               │
  IR: NormalizedSource           clean text, no comments,
      + LineMap                  no continuation lines.
                                 LineMap[normalized_line] → original_line
               │
               ▼
┌───────────────────────────────────────┐
│  3b: Include Resolution              │
│                                       │
│  File-level operation. Opens files,   │
│  pastes contents inline.              │
│                                       │
│  - Resolve #include "file"            │
│    (search relative to current file,  │
│     then -I paths)                    │
│  - Resolve #include <file>            │
│    (search -I paths only)             │
│  - Recursively normalize + include    │
│    nested files                       │
│  - Track #pragma once                 │
│    (mark file path as included)       │
│  - Recognize include guard pattern    │
│    (#ifndef X / #define X at file     │
│     top, #endif at file bottom)       │
│    to avoid re-inclusion              │
│  - All other # directives pass        │
│    through UNCHANGED                  │
└──────────────┬────────────────────────┘
               │
  IR: TranslationUnit           single text with all includes
                                 inlined. Still contains #define,
                                 #ifdef, and macro invocations.
                                 File boundary markers (# line
                                 directives) inserted for error
                                 reporting.
               │
               ▼
┌───────────────────────────────────────┐
│  3c: Conditional Compilation          │
│      + Macro Definition               │
│                                       │
│  Single pass. These two are coupled:  │
│  #ifdef checks whether a macro is     │
│  #defined, so definitions must be     │
│  tracked here.                        │
│                                       │
│  - Process #define (store in macro    │
│    table: name, params, body)         │
│  - Process #undef (remove from table) │
│  - Evaluate #ifdef / #ifndef          │
│  - Evaluate #if / #elif               │
│    (constant expressions with         │
│     defined() operator)               │
│  - Process #else / #endif             │
│  - Handle nesting                     │
│  - Strip dead branches                │
│  - Pass through #pragma GCC           │
│    diagnostic (or discard)            │
│  - Emit live code (with macro         │
│    invocations still unresolved)      │
└──────────────┬────────────────────────┘
               │
  IR: ConditionalResult         text with all conditionals
      + MacroTable               resolved, dead branches removed.
                                 No more # directives remain.
                                 MacroTable = HashMap<String, Macro>
                                 where Macro = {
                                   name, is_function_like,
                                   params: Array<String>,
                                   body: String
                                 }
               │
               ▼
┌───────────────────────────────────────┐
│  3d: Macro Expansion                  │
│                                       │
│  Pure text transformation using the   │
│  MacroTable from 3c. No file I/O,    │
│  no conditionals, no directives.      │
│                                       │
│  - Object-like expansion:             │
│    FOO → replacement text             │
│  - Function-like expansion:           │
│    FOO(a, b) → body with args         │
│    substituted                        │
│  - ## token pasting: a ## b → ab      │
│  - # stringification: #x → "x"       │
│  - __VA_ARGS__ substitution           │
│  - Recursion prevention               │
│    (macro cannot expand itself)       │
│  - Predefined macros:                 │
│    __FILE__, __LINE__                 │
└──────────────┬────────────────────────┘
               │
  IR: ExpandedSource            fully expanded C source.
                                 No # directives, no macro
                                 invocations. Ready for
                                 tokenizer.
               │
               ▼
          (Tokenizer)
```

### Tokenizer / Lexer Split

The tokenizer and lexer are separate passes with distinct responsibilities:

```
                   Tokenizer (Pass 1)              Lexer (Pass 2)
                   ──────────────────              ────────────────
Knowledge:         Language-agnostic               C-specific
Input:             char stream                     Array<RawToken>
Output:            Array<RawToken>                 Array<LexToken>

WORD "int"         → RAW_WORD                      → LEX_KEYWORD_INT
WORD "foo"         → RAW_WORD                      → LEX_IDENTIFIER
"42"               → RAW_NUMBER                    → LEX_INT_LITERAL (value=42)
"0x2A"             → RAW_NUMBER                    → LEX_INT_LITERAL (value=42)
"3.14"             → RAW_NUMBER                    → LEX_FLOAT_LITERAL (value=3.14)
"hello\n"          → RAW_STRING (raw bytes)        → LEX_STRING_LITERAL (escapes resolved)
'a'                → RAW_CHAR (raw bytes)          → LEX_CHAR_LITERAL (value=97)
==                 → RAW_SYMBOL_EQ_EQ              → LEX_EQ
->                 → RAW_SYMBOL_ARROW              → LEX_ARROW
// comment         → (skipped)                     → (not present)
/* comment */      → (skipped)                     → (not present)
```

Both passes produce flat arrays that serialize to disk:

```
.tokens file (Pass 1 output):
┌──────────────────────────────────────────────┐
│ Header: magic("FUNTOK"), version(u16),       │
│         source_length(u32),                  │
│         token_count(u32)                     │
├──────────────────────────────────────────────┤
│ RawToken[0]: type(u8) offset(u32) len(u16)  │
│              line(u32) col(u16)              │
├──────────────────────────────────────────────┤
│ RawToken[1]: ...                             │
└──────────────────────────────────────────────┘
  Tokens reference byte offsets in the original source file.
  The source file is needed to interpret token values.

.lex file (Pass 2 output):
┌──────────────────────────────────────────────┐
│ Header: magic("FUNLEX"), version(u16),       │
│         source_length(u32),                  │
│         token_count(u32),                    │
│         string_table_size(u32)               │
├──────────────────────────────────────────────┤
│ LexToken[0]: type(u16) offset(u32) len(u16) │
│              line(u32) col(u16)              │
│              value(u64)                      │
├──────────────────────────────────────────────┤
│ LexToken[1]: ...                             │
├──────────────────────────────────────────────┤
│ String table: resolved string/char literal   │
│ values (null-separated)                      │
└──────────────────────────────────────────────┘
  value field holds: integer value for int literals,
  double bits for float literals, string table offset
  for string/char literals, 0 for keywords/operators.
```

### Semantic Analysis — Four-Stage Split

Semantic analysis is split into four distinct passes, each with serializable output.
The ordering is load-bearing: each stage depends on the previous stage's output.

```
                    ┌─────────────────────────┐
                    │  6a: Symbol Collection  │
                    │                         │
                    │  First pass over AST.   │
                    │  Collects all top-level  │
                    │  declarations:          │
                    │  - function prototypes  │
                    │  - function definitions │
                    │  - global variables     │
                    │  - struct/union/enum    │
                    │    definitions          │
                    │  - typedefs             │
                    │                         │
                    │  Handles forward decls. │
                    │  Does NOT enter         │
                    │  function bodies.       │
                    └────────────┬────────────┘
                                 │
         IR: AST + SymbolTable   │  SymbolTable = HashMap<String, Symbol>
             (file-scope only)   │  per scope level. At this stage, only
                                 │  the global scope is populated.
                                 │
                                 │  Symbol = {
                                 │    name, kind (function/variable/type/enum),
                                 │    declaration_node (AST pointer),
                                 │    type (unresolved — may reference typedef names)
                                 │  }
                                 ▼
                    ┌─────────────────────────┐
                    │  6b: Type Resolution    │
                    │                         │
                    │  Resolves typedef       │
                    │  chains to concrete     │
                    │  types.                 │
                    │                         │
                    │  Computes struct/union   │
                    │  layouts:               │
                    │  - field offsets        │
                    │  - alignment            │
                    │  - padding              │
                    │  - total size           │
                    │                         │
                    │  Assigns enum values    │
                    │  (explicit + implicit). │
                    │                         │
                    │  Builds pointer/array   │
                    │  type chains.           │
                    └────────────┬────────────┘
                                 │
         IR: AST + SymbolTable   │  TypeGraph = all Type nodes, interlinked.
             + TypeGraph         │
                                 │  Type = {
                                 │    kind, size_bytes, alignment,
                                 │    union {
                                 │      pointer: { pointee: *Type },
                                 │      array:   { element: *Type, count },
                                 │      record:  { fields: Array<Field> },
                                 │      enum:    { values: Array<EnumValue> },
                                 │      function:{ return_type, params: Array<Type> }
                                 │    },
                                 │    is_const
                                 │  }
                                 │
                                 │  Field = { name, type: *Type, offset, size }
                                 │  EnumValue = { name, value: int64 }
                                 ▼
                    ┌─────────────────────────┐
                    │  6c: Name Resolution    │
                    │                         │
                    │  Walks function bodies. │
                    │  For each scope:        │
                    │  - push scope on enter  │
                    │  - resolve identifiers  │
                    │    to symbol table      │
                    │    entries              │
                    │  - pop scope on exit    │
                    │                         │
                    │  Tags each AST_IDENT    │
                    │  node with a pointer    │
                    │  to its Symbol entry.   │
                    │                         │
                    │  Reports: undeclared    │
                    │  identifier errors.     │
                    └────────────┬────────────┘
                                 │
         IR: ResolvedAST         │  ResolvedAST = same tree structure, but every
             + SymbolTable       │  AST_IDENT_EXPR node now has a .symbol field
             + TypeGraph         │  pointing to its Symbol entry. Scopes are fully
                                 │  populated (globals + locals + block-locals).
                                 │
                                 │  Scope stack is materialized: each function body
                                 │  has its own scope chain preserved for later use
                                 │  by the type checker.
                                 ▼
                    ┌─────────────────────────┐
                    │  6d: Type Checking      │
                    │                         │
                    │  Walks the resolved AST.│
                    │  For each expression:   │
                    │  - computes result type │
                    │  - verifies operand     │
                    │    compatibility        │
                    │  - inserts implicit     │
                    │    conversions          │
                    │                         │
                    │  Checks:                │
                    │  - assignment types     │
                    │  - function call args   │
                    │  - return types         │
                    │  - member access valid  │
                    │  - array index is int   │
                    │  - pointer arithmetic   │
                    │                         │
                    │  Tags each expression   │
                    │  node with .result_type │
                    └────────────┬────────────┘
                                 │
         IR: TypedAST            │  TypedAST = every expression node has a
             + SymbolTable       │  .result_type: *Type field. Implicit conversion
             + TypeGraph         │  nodes (AST_IMPLICIT_CAST) are inserted where
                                 │  needed (e.g., int → int64_t promotion).
                                 │
                                 │  This is the final IR before code generation.
                                 │  The code generator can trust that all types
                                 │  are resolved and all names are bound.
                                 ▼
                           (Code Generator)
```

### Grammar as Documentation (Option C)

Token patterns are defined in a grammar file (`src/compiler/tokenizer.grammar`)
that serves as the specification. The tokenizer itself is hand-written for
performance and error quality. The grammar file is used as a test oracle —
a test harness verifies that the hand-written tokenizer matches the grammar
for all patterns.

```
tokenizer.grammar   ←── human-readable specification
tokenizer.c         ←── hand-written implementation
test_tokenizer.c    ←── tests verify implementation matches grammar
```

This gives us:
- **Declarative spec**: grammar file documents exactly what patterns are recognized
- **Implementation freedom**: hand-written code for speed and error messages
- **Correctness assurance**: tests enforce the grammar is the source of truth

### File Structure

```
src/
  compiler/
    # Preprocessor (4 stages)
    pp_normalize.c / .h      Stage 3a: line splicing, comment removal, CRLF→LF
    pp_include.c / .h        Stage 3b: #include resolution, pragma once, guards
    pp_conditional.c / .h    Stage 3c: #ifdef/#if + #define/#undef processing
    pp_expand.c / .h         Stage 3d: macro expansion (##, __VA_ARGS__, etc.)
    macro.c / .h             MacroTable data structure and operations

    # Front-end (tokenizer + lexer + parser)
    tokenizer.grammar        Token pattern specification (test oracle)
    tokenizer.c / .h         character stream → raw tokens (hand-written)
    lexer.c / .h             raw tokens → classified C tokens
    token.c / .h             Token types, serialization, deserialization
    parser.c / .h            Recursive descent parser
    ast.c / .h               AST node types and constructors

    # Semantic analysis (4 stages)
    symcollect.c / .h        Stage 6a: collect top-level declarations
    typeresolve.c / .h       Stage 6b: resolve typedefs, compute struct layouts
    nameresolve.c / .h       Stage 6c: resolve identifiers to symbols in scopes
    typecheck.c / .h         Stage 6d: verify type compatibility, tag expressions
    type.c / .h              Type representation and operations
    symtab.c / .h            Symbol table (scoped name → type mapping)

    # Back-end
    codegen.c / .h           x86-64 assembly emission
    codegen_win64.c / .h     Win64 ABI specifics
    codegen_sysv.c / .h      System V ABI specifics

    # Infrastructure
    serialize.c / .h         Binary serialization for all intermediate formats
    linemap.c / .h           Original line number tracking across transformations
    compiler.c / .h          Top-level pipeline driver (orchestrates all stages)

  commands/
    cmd_compile.c / .h       The `fun compile` command
    cmd_inspect.c / .h       The `fun inspect-*` commands
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

### Decision 4: Two-pass tokenizer/lexer split with serializable intermediate results

**Rationale:** Separating tokenization (language-agnostic character pattern recognition) from lexing (C-specific classification) gives us:
- A reusable tokenizer that could handle other C-like languages
- Independently testable passes with clear contracts
- Serializable intermediate results (`.tokens` and `.lex` files) for debugging, caching, and tooling
- A clean boundary: the tokenizer never needs to know what `int` means

**Alternatives considered:**
- Single-pass lexer (traditional): Simpler but couples character scanning with language semantics, no intermediate serialization point
- Three passes (scanner + tokenizer + lexer): Over-split, the scanner/tokenizer boundary is not meaningful

### Decision 5: Grammar file as documentation and test oracle (Option C)

**Rationale:** Token patterns are defined declaratively in `tokenizer.grammar` as the specification. The tokenizer is hand-written for performance, error quality, and fundamental library compatibility. A test harness verifies the implementation matches the grammar. This gives us a declarative spec without the complexity of building a grammar-to-code generator.

**Alternatives considered:**
- Generated tokenizer from grammar (Option B): Requires building a regex-to-C compiler, which is a project within the project
- Hand-written only (Option A): Patterns are implicit in code, harder to review and verify
- Hybrid with generated tests (chosen): Best of both — spec is readable, implementation is fast, tests enforce correctness

### Decision 6: Build phases in recommended order (tokenizer first, not preprocessor)

**Rationale:** The tokenizer is immediately testable on hand-written input, produces visible results fast, and forces early definition of token types that all later phases depend on. The preprocessor is complex and can be deferred — during development, GCC's `-E` flag can provide preprocessed input for testing.

**Alternatives considered:**
- Preprocessor first: Theoretically cleaner pipeline, but weeks of work before any visible progress
- All phases in parallel: Too many interdependencies

### Decision 7: Fundamental library additions handled in fundamental repo

**Rationale:** Character classification (`is_digit`, `is_alpha`), substring extraction, and similar utilities belong in the fundamental string module, not as compiler-specific code.

**Additions needed in fundamental:**
- `fun_string_char_is_digit(char c)` — and similar classification functions
- `fun_string_substring(source, start, length, output, output_size)` — extract substring
- These are vendored into fundamental-cli once available

**Workaround until available:** `DEFINE_ARRAY_TYPE(char)` for growable buffers; inline char classification is trivial.

## Data Structures

### RawToken (Pass 1 — Tokenizer output)

```c
typedef enum {
    // Structural categories — no language knowledge
    RAW_WORD,              // [a-zA-Z_][a-zA-Z0-9_]*
    RAW_NUMBER,            // digit sequences (dec, hex, oct, float)
    RAW_STRING,            // "..." (raw bytes, escapes unprocessed)
    RAW_CHAR,              // '...' (raw bytes, escapes unprocessed)

    // Single-character symbols
    RAW_PLUS,              // +
    RAW_MINUS,             // -
    RAW_STAR,              // *
    RAW_SLASH,             // /
    RAW_PERCENT,           // %
    RAW_AMPERSAND,         // &
    RAW_PIPE,              // |
    RAW_CARET,             // ^
    RAW_TILDE,             // ~
    RAW_BANG,              // !
    RAW_ASSIGN,            // =
    RAW_LT,               // <
    RAW_GT,                // >
    RAW_DOT,               // .
    RAW_QUESTION,          // ?
    RAW_COLON,             // :
    RAW_SEMICOLON,         // ;
    RAW_COMMA,             // ,
    RAW_LPAREN,            // (
    RAW_RPAREN,            // )
    RAW_LBRACE,            // {
    RAW_RBRACE,            // }
    RAW_LBRACKET,          // [
    RAW_RBRACKET,          // ]
    RAW_HASH,              // #

    // Multi-character symbols
    RAW_PLUS_ASSIGN,       // +=
    RAW_MINUS_ASSIGN,      // -=
    RAW_STAR_ASSIGN,       // *=
    RAW_SLASH_ASSIGN,      // /=
    RAW_PERCENT_ASSIGN,    // %=
    RAW_AMP_ASSIGN,        // &=
    RAW_PIPE_ASSIGN,       // |=
    RAW_CARET_ASSIGN,      // ^=
    RAW_LSHIFT_ASSIGN,     // <<=
    RAW_RSHIFT_ASSIGN,     // >>=
    RAW_EQ_EQ,             // ==
    RAW_BANG_EQ,           // !=
    RAW_LT_EQ,            // <=
    RAW_GT_EQ,             // >=
    RAW_AMP_AMP,           // &&
    RAW_PIPE_PIPE,         // ||
    RAW_LSHIFT,            // <<
    RAW_RSHIFT,            // >>
    RAW_PLUS_PLUS,         // ++
    RAW_MINUS_MINUS,       // --
    RAW_ARROW,             // ->
    RAW_ELLIPSIS,          // ...

    // Special
    RAW_EOF,
} RawTokenType;

typedef struct {
    RawTokenType type;
    uint32_t     offset;     // byte offset in source buffer
    uint16_t     length;     // byte length of token text
    uint32_t     line;
    uint16_t     col;
} RawToken;
```

### LexToken (Pass 2 — Lexer output)

```c
typedef enum {
    // Keywords (classified from RAW_WORD)
    LEX_INT, LEX_CHAR, LEX_VOID, LEX_FLOAT, LEX_DOUBLE,
    LEX_STRUCT, LEX_UNION, LEX_ENUM, LEX_TYPEDEF,
    LEX_IF, LEX_ELSE, LEX_FOR, LEX_WHILE, LEX_DO,
    LEX_SWITCH, LEX_CASE, LEX_DEFAULT,
    LEX_BREAK, LEX_CONTINUE, LEX_RETURN,
    LEX_STATIC, LEX_CONST, LEX_INLINE, LEX_EXTERN,
    LEX_SIZEOF,

    // Identifier (RAW_WORD that is not a keyword)
    LEX_IDENTIFIER,

    // Literals (classified from RAW_NUMBER, RAW_STRING, RAW_CHAR)
    LEX_INT_LITERAL,       // value field = parsed integer
    LEX_FLOAT_LITERAL,     // value field = double bits (via union)
    LEX_CHAR_LITERAL,      // value field = resolved char value
    LEX_STRING_LITERAL,    // value field = string table offset

    // Operators — carried forward from tokenizer with semantic names
    LEX_PLUS, LEX_MINUS, LEX_STAR, LEX_SLASH, LEX_PERCENT,
    LEX_AMPERSAND, LEX_PIPE, LEX_CARET, LEX_TILDE,
    LEX_BANG, LEX_ASSIGN, LEX_LT, LEX_GT,
    LEX_DOT, LEX_QUESTION, LEX_COLON,

    LEX_PLUS_ASSIGN, LEX_MINUS_ASSIGN, LEX_STAR_ASSIGN,
    LEX_SLASH_ASSIGN, LEX_PERCENT_ASSIGN,
    LEX_AMP_ASSIGN, LEX_PIPE_ASSIGN, LEX_CARET_ASSIGN,
    LEX_LSHIFT_ASSIGN, LEX_RSHIFT_ASSIGN,
    LEX_EQ, LEX_NEQ, LEX_LTE, LEX_GTE,
    LEX_AND, LEX_OR,
    LEX_LSHIFT, LEX_RSHIFT,
    LEX_INCREMENT, LEX_DECREMENT,
    LEX_ARROW,

    // Punctuation
    LEX_LPAREN, LEX_RPAREN,
    LEX_LBRACE, LEX_RBRACE,
    LEX_LBRACKET, LEX_RBRACKET,
    LEX_SEMICOLON, LEX_COMMA,
    LEX_ELLIPSIS,

    // Special
    LEX_EOF,
} LexTokenType;

typedef struct {
    LexTokenType type;
    uint32_t     offset;     // byte offset in original source
    uint16_t     length;     // byte length in original source
    uint32_t     line;
    uint16_t     col;
    uint64_t     value;      // parsed int, double bits, or string table offset
} LexToken;
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

## Pipeline Intermediate Results — Complete Reference

Every stage consumes one IR and produces the next. This table defines each
intermediate result, its contents, and its serialization format.

```
Stage            IR Name             Contents                       Disk Format
─────            ───────             ────────                       ───────────

                  ── PREPROCESSOR ──

Normalize        NormalizedSource    clean text (no comments,       .norm (text)
                                     no continuation lines)
                 + LineMap           normalized_line → original_line

Include          TranslationUnit    single text with all includes   .inc (text)
                                     inlined, # line markers for
                                     error reporting

Conditional      ConditionalResult  text with dead branches         .cond (text)
                 + MacroTable        removed, no # directives
                                     MacroTable = HashMap<String,
                                       Macro{name, params, body}>

Macro Expand     ExpandedSource     fully expanded C source         .i (text)
                                     (no macros, no directives)

                  ── FRONT-END ──

Tokenizer        RawTokenStream     Array<RawToken>                 .tokens (bin)
                                     + source buffer reference

Lexer            LexTokenStream     Array<LexToken>                 .lex (bin)
                                     + StringTable (resolved
                                       string/char literals)

Parser           AST                Tree of ASTNode                 .ast (bin)
                                     (untyped, unresolved)

                  ── SEMANTIC ANALYSIS ──

Sym Collect      SymbolAST          AST (unchanged)                 .sym (bin)
                                     + SymbolTable (global scope)

Type Resolve     TypedSymbolAST     AST (unchanged)                 .types (bin)
                                     + SymbolTable (types resolved)
                                     + TypeGraph (all Type nodes)

Name Resolve     ResolvedAST        AST (ident nodes tagged with    .resolved (bin)
                                      .symbol pointers)
                                     + SymbolTable (all scopes)
                                     + TypeGraph

Type Check       TypedAST           AST (expression nodes tagged    .typed (bin)
                                      with .result_type)
                                     + implicit cast nodes inserted
                                     + SymbolTable (all scopes)
                                     + TypeGraph

                  ── BACK-END ──

Code Gen         Assembly           AT&T syntax x86-64 asm          .s (text)

Assemble         ObjectCode         Machine code (via GNU as)       .o (bin)

Link             Executable         PE (Windows) or ELF (Linux)     .exe / no ext
```

### Serialization Format Summary

All binary formats share a common header structure:

```
Common header (12 bytes):
  magic:    6 bytes    Format identifier (e.g., "FUNTOK", "FUNLEX", "FUNAST")
  version:  u16        Format version (for forward compatibility)
  flags:    u32        Reserved / format-specific flags

Format-specific header follows, then data payload.
```

Preprocessor formats (.norm, .inc, .cond, .i) are plain text — no binary
serialization needed. They are human-readable directly.

| Format   | Magic    | Header extension                          | Payload                                    |
|----------|----------|-------------------------------------------|--------------------------------------------|
| .tokens  | FUNTOK   | source_len(u32), count(u32)               | RawToken[count]: type(u8) offset(u32) len(u16) line(u32) col(u16) |
| .lex     | FUNLEX   | source_len(u32), count(u32), strtab_sz(u32) | LexToken[count]: type(u16) offset(u32) len(u16) line(u32) col(u16) value(u64); then strtab bytes |
| .ast     | FUNAST   | node_count(u32)                           | Flattened AST nodes (pre-order traversal, child counts for reconstruction) |
| .sym     | FUNSYM   | scope_count(u32), symbol_count(u32)       | Scope descriptors + Symbol entries (name offset into name table, kind, type_id) |
| .types   | FUNTYP   | type_count(u32)                           | Type entries (kind, size, alignment, union payload); cross-references by type_id |
| .resolved| FUNRES   | Same as .ast + .sym                       | AST nodes with symbol_id fields populated |
| .typed   | FUNTYD   | Same as .resolved + type refs             | AST nodes with type_id fields populated; includes inserted implicit cast nodes |

## Risks / Trade-offs

**[C declaration syntax is notoriously hard to parse]** → Mitigation: Recursive descent with a "declaration specifiers + declarator" model (the standard approach). The subset excludes the worst cases (no K&R style, no complex function pointer nesting beyond what fundamental uses).

**[Preprocessor macro expansion is a compiler in itself]** → Mitigation: Target only the macros actually used (DEFINE_RESULT_TYPE, DEFINE_ARRAY_TYPE, DEFINE_HASHMAP_TYPE, etc.). These use `##` and `__VA_ARGS__` but are structurally regular.

**[Two ABIs (Win64 and System V) doubles codegen testing]** → Mitigation: Separate ABI modules with shared core. Test on the native platform first, cross-platform second.

**[Binary size increase]** → Mitigation: The compiler adds significant code to `fun.exe`. Acceptable trade-off for self-contained toolchain. Can be made optional (compile out if not needed).

**[No optimization = slow generated code]** → Mitigation: Correct code first. Optimization is a separate future change. Users who need performance can still use GCC.

## Phased Implementation Order

```
Phase 1a:  Tokenizer + RawToken types + grammar file
           → hand-written tokenizer, grammar as spec/test oracle
           → testable immediately with hand-written input

Phase 1b:  Lexer + LexToken types
           → classifies raw tokens with C knowledge
           → testable by chaining after tokenizer

Phase 2:   Parser (expressions only)
           → testable with expression strings

Phase 3a:  Preprocessor: source normalization
           → comment removal, line splicing, CRLF normalization

Phase 3b:  Preprocessor: include resolution
           → #include, #pragma once, include guards

Phase 4:   Parser (declarations + statements)
           → can parse full .c files (using gcc -E for preprocessing)

Phase 5a:  Preprocessor: conditional compilation + macro definitions
           → #ifdef, #if, #define, #undef, dead branch removal

Phase 5b:  Preprocessor: macro expansion
           → object-like, function-like, ##, __VA_ARGS__
           → can now preprocess real files without gcc -E

Phase 6a:  Symbol collection
           → collect all top-level declarations from AST

Phase 6b:  Type resolution
           → resolve typedefs, compute struct layouts, enum values

Phase 6c:  Name resolution
           → resolve identifiers in function bodies to symbols

Phase 6d:  Type checking
           → verify type compatibility, tag expressions with types

Phase 7:   Code generator (expressions + functions)
           → can compile minimal programs (main returning 42)

Phase 8:   Code generator (control flow + structs)
           → can compile non-trivial programs

Phase 9:   Integration with fun build
           → fun build --compiler=funcc as alternative to gcc

Phase 10:  Validation
           → compile fundamental-cli with funcc, compare output
```

## Resolved Questions

**Should funcc be a separate binary or a `fun compile` subcommand?**
→ Subcommand. `fun compile <file.c>` fits the existing CLI pattern.

**Should the preprocessor output be cacheable for incremental builds?**
→ Yes. Preprocessed output is cacheable. Combined with `.tokens` and `.lex` caching, this enables incremental builds where only changed files are reprocessed.

**What assembler to target?**
→ GNU `as` (available via MinGW on Windows, native on Linux). No NASM dependency.

**Should every pipeline stage be writable to disk?**
→ Yes. This is a learning project — every stage produces inspectable output:
- `fun compile --norm file.c` → `.norm` normalized source (text)
- `fun compile --inc file.c` → `.inc` includes resolved (text)
- `fun compile --cond file.c` → `.cond` conditionals resolved (text)
- `fun compile -E file.c` → `.i` fully preprocessed source (text)
- `fun compile --tokens file.c` → `.tokens` binary file
- `fun compile --lex file.c` → `.lex` binary file
- `fun compile --ast file.c` → `.ast` binary file
- `fun compile --symbols file.c` → `.sym` binary file
- `fun compile --types file.c` → `.types` binary file
- `fun compile --resolved file.c` → `.resolved` binary file
- `fun compile --typed file.c` → `.typed` binary file
- `fun compile -S file.c` → `.s` assembly file (text)
- `fun compile -c file.c` → `.o` object file (via GNU as)
- `fun compile file.c` → full pipeline to executable

**Should `.tokens` and `.lex` files use text or binary format?**
→ Binary format for all intermediate results (compact, fast to read/write). Separate `fun` subcommands to view them in human-readable text format:
- `fun inspect-tokens <file.tokens>` → human-readable raw token dump
- `fun inspect-lex <file.lex>` → human-readable classified token dump
- `fun inspect-ast <file.ast>` → human-readable AST tree dump
- `fun inspect-sym <file.sym>` → human-readable symbol table dump
- `fun inspect-types <file.types>` → human-readable type graph dump

**When should self-hosting become a goal?**
→ Eventually. Not part of this change. Self-hosting is a future milestone after funcc can compile fundamental-cli correctly.
