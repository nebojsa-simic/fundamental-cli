## Phase 1a: Tokenizer (Pass 1 — language-agnostic)

- [ ] 1a.1 Write tokenizer.grammar: define all token patterns (WORD, NUMBER, STRING, CHAR, symbols)
- [ ] 1a.2 Define RawTokenType enum (RAW_WORD, RAW_NUMBER, RAW_STRING, RAW_CHAR, all symbols, RAW_EOF)
- [ ] 1a.3 Define RawToken struct (type, offset, length, line, col)
- [ ] 1a.4 Implement tokenizer initialization (accept source buffer + length, init cursor state)
- [ ] 1a.5 Implement whitespace consumption with line/column tracking
- [ ] 1a.6 Implement comment skipping (// line comments and /* block comments */)
- [ ] 1a.7 Implement WORD tokenization: [a-zA-Z_][a-zA-Z0-9_]*
- [ ] 1a.8 Implement NUMBER tokenization: decimal, hex (0x/0X), octal (0), float (with .)
- [ ] 1a.9 Implement STRING tokenization: "..." (raw bytes, track escape sequences but don't resolve)
- [ ] 1a.10 Implement CHAR tokenization: '...' (raw bytes, track escape sequences but don't resolve)
- [ ] 1a.11 Implement single-character symbol tokenization (all punctuation and operators)
- [ ] 1a.12 Implement multi-character symbol tokenization (==, !=, <=, >=, &&, ||, <<, >>, ->, +=, -=, etc.)
- [ ] 1a.13 Implement ellipsis (...) tokenization
- [ ] 1a.14 Implement tokenizer error reporting (unexpected character, unterminated string/char/comment)
- [ ] 1a.15 Implement .tokens serialization (write Array<RawToken> to binary file)
- [ ] 1a.16 Implement .tokens deserialization (read binary file to Array<RawToken>)
- [ ] 1a.17 Write tokenizer tests: verify against tokenizer.grammar patterns
- [ ] 1a.18 Write tokenizer tests: tokenize hand-written C fragments, verify token types and positions
- [ ] 1a.19 Write tokenizer tests: tokenize actual fundamental-cli source files (post gcc -E), verify no errors

## Phase 1b: Lexer (Pass 2 — C-specific classification)

- [ ] 1b.1 Define LexTokenType enum (all C keywords, LEX_IDENTIFIER, literal types, operators, punctuation)
- [ ] 1b.2 Define LexToken struct (type, offset, length, line, col, value)
- [ ] 1b.3 Implement keyword lookup table (RAW_WORD text → LexTokenType keyword or LEX_IDENTIFIER)
- [ ] 1b.4 Implement integer literal parsing (decimal, hex, octal string → uint64_t value)
- [ ] 1b.5 Implement float literal parsing (string → double value, stored as uint64_t bits)
- [ ] 1b.6 Implement character literal resolution (escape sequences → byte value)
- [ ] 1b.7 Implement string literal resolution (escape sequences → resolved bytes, store in string table)
- [ ] 1b.8 Implement operator/symbol mapping (RAW_* → LEX_* with semantic names)
- [ ] 1b.9 Implement lexer driver: iterate Array<RawToken>, classify each, produce Array<LexToken>
- [ ] 1b.10 Implement .lex serialization (write Array<LexToken> + string table to binary file)
- [ ] 1b.11 Implement .lex deserialization (read binary file to Array<LexToken> + string table)
- [ ] 1b.12 Write lexer tests: verify keyword classification for all C keywords
- [ ] 1b.13 Write lexer tests: verify numeric literal parsing (dec, hex, oct, float)
- [ ] 1b.14 Write lexer tests: verify escape sequence resolution (\n, \t, \0, \\, \', \", \x41)
- [ ] 1b.15 Write lexer tests: full pipeline (source → tokenize → lex) on fundamental-cli files

## Phase 2: Parser — Expressions

- [ ] 2.1 Define ASTNodeType enum for all node types
- [ ] 2.2 Define AST node struct (tagged union or struct-per-type)
- [ ] 2.3 Implement AST node constructors and destructor (free tree recursively)
- [ ] 2.4 Implement parser infrastructure (token cursor, peek, advance, expect, error reporting)
- [ ] 2.5 Implement primary expression parsing (literals, identifiers, parenthesized expressions)
- [ ] 2.6 Implement postfix expressions (function calls, array indexing, member access . and ->, post-increment/decrement)
- [ ] 2.7 Implement unary expressions (prefix -, !, ~, &, *, ++, --, sizeof, casts)
- [ ] 2.8 Implement binary expressions with precedence climbing (arithmetic, bitwise, logical, comparison, assignment, compound assignment)
- [ ] 2.9 Implement ternary expression (? :)
- [ ] 2.10 Implement comma expression
- [ ] 2.11 Implement compound literal parsing: (Type){ .field = value }
- [ ] 2.12 Write parser tests: parse expression strings, verify AST structure
- [ ] 2.13 Implement AST pretty-printer for debugging

## Phase 3a: Preprocessor — Source Normalization

- [ ] 3a.1 Implement file reader (load source file into memory buffer using fun_read_file_in_memory)
- [ ] 3a.2 Implement output buffer (DEFINE_ARRAY_TYPE(char) for growable text)
- [ ] 3a.3 Implement line ending normalization (CRLF → LF)
- [ ] 3a.4 Implement backslash-newline joining (line splicing)
- [ ] 3a.5 Implement // line comment removal
- [ ] 3a.6 Implement /* block comment */ removal (preserve newlines for line counting)
- [ ] 3a.7 Implement LineMap: track normalized_line → original_line mapping
- [ ] 3a.8 Handle edge cases: comment inside string literal (don't remove), unterminated block comment (error)
- [ ] 3a.9 Implement .norm file output (write normalized text to disk)
- [ ] 3a.10 Write tests: normalize files with mixed CRLF/LF, verify output
- [ ] 3a.11 Write tests: normalize files with continuation lines, verify joining
- [ ] 3a.12 Write tests: normalize files with comments, verify removal
- [ ] 3a.13 Write tests: verify strings containing // and /* are preserved
- [ ] 3a.14 Write tests: normalize actual fundamental-cli source files, verify no errors

## Phase 3b: Preprocessor — Include Resolution

- [ ] 3b.1 Implement #include "file" resolution (search relative to current file, then -I paths)
- [ ] 3b.2 Implement #include <file> resolution (search -I paths only)
- [ ] 3b.3 Implement recursive include processing (normalize + inline nested includes)
- [ ] 3b.4 Implement include depth limit (detect infinite recursion, e.g., max 64 levels)
- [ ] 3b.5 Implement #pragma once tracking (set of included file paths, skip if already seen)
- [ ] 3b.6 Implement include guard pattern recognition (#ifndef X / #define X at top, #endif at bottom)
- [ ] 3b.7 Insert # line markers at file boundaries (for error reporting: "file.h:42")
- [ ] 3b.8 Pass through all non-#include directives unchanged
- [ ] 3b.9 Implement .inc file output (write translation unit to disk)
- [ ] 3b.10 Write tests: resolve simple #include "file" chains
- [ ] 3b.11 Write tests: verify #pragma once prevents re-inclusion
- [ ] 3b.12 Write tests: verify include guard pattern prevents re-inclusion
- [ ] 3b.13 Write tests: verify include depth limit triggers error
- [ ] 3b.14 Write tests: resolve actual fundamental-cli include chains (vendor/fundamental/include/...)

## Phase 4: Parser — Declarations + Statements

- [ ] 4.1 Implement declaration specifier parsing (static, const, inline, extern, type specifiers)
- [ ] 4.2 Implement declarator parsing (identifiers, pointer declarators, array declarators, function declarators)
- [ ] 4.3 Implement struct definition parsing (struct keyword, name, { field-list })
- [ ] 4.4 Implement union definition parsing
- [ ] 4.5 Implement enum definition parsing (enum keyword, name, { enumerator-list })
- [ ] 4.6 Implement typedef parsing
- [ ] 4.7 Implement function pointer type parsing
- [ ] 4.8 Implement variable declaration parsing (with optional initializer, designated initializers)
- [ ] 4.9 Implement function definition parsing (declaration + compound statement)
- [ ] 4.10 Implement compound statement (block) parsing: { declarations-and-statements }
- [ ] 4.11 Implement if/else statement parsing
- [ ] 4.12 Implement for statement parsing
- [ ] 4.13 Implement while statement parsing
- [ ] 4.14 Implement do-while statement parsing
- [ ] 4.15 Implement switch/case/default statement parsing
- [ ] 4.16 Implement break, continue, return statement parsing
- [ ] 4.17 Implement expression statement parsing
- [ ] 4.18 Implement translation unit parsing (sequence of top-level declarations)
- [ ] 4.19 Write parser tests: parse actual fundamental-cli source files (preprocessed), verify no errors
- [ ] 4.20 Test round-trip: parse → pretty-print → parse → compare ASTs

## Phase 5a: Preprocessor — Conditional Compilation + Macro Definitions

- [ ] 5a.1 Define Macro struct (name, is_function_like, params: Array<String>, body: String)
- [ ] 5a.2 Implement MacroTable (HashMap<String, Macro>)
- [ ] 5a.3 Implement #define processing: parse object-like macro, store in MacroTable
- [ ] 5a.4 Implement #define processing: parse function-like macro (with parameter list), store
- [ ] 5a.5 Implement #undef processing: remove from MacroTable
- [ ] 5a.6 Implement #ifdef / #ifndef evaluation (check MacroTable for name existence)
- [ ] 5a.7 Implement conditional nesting stack (track if/elif/else/endif depth)
- [ ] 5a.8 Implement #else processing (invert current conditional state)
- [ ] 5a.9 Implement #endif processing (pop conditional stack)
- [ ] 5a.10 Implement #if / #elif constant expression evaluation
- [ ] 5a.11 Implement defined() operator in #if expressions
- [ ] 5a.12 Implement dead branch stripping (skip lines inside false conditionals)
- [ ] 5a.13 Implement #pragma GCC diagnostic handling (pass through or discard)
- [ ] 5a.14 Implement .cond file output (write conditional-free text + macro table to disk)
- [ ] 5a.15 Write tests: #ifdef with defined/undefined macros
- [ ] 5a.16 Write tests: nested #ifdef/#ifndef/#if/#elif/#else/#endif
- [ ] 5a.17 Write tests: #if with arithmetic expressions and defined()
- [ ] 5a.18 Write tests: verify dead branches are completely removed
- [ ] 5a.19 Write tests: process actual fundamental-cli conditionals (header guards, platform checks)

## Phase 5b: Preprocessor — Macro Expansion

- [ ] 5b.1 Implement object-like macro expansion (scan text for macro names, substitute body)
- [ ] 5b.2 Implement function-like macro argument parsing (match FOO( ... , ... ) invocations)
- [ ] 5b.3 Implement function-like macro body substitution (replace params with arguments)
- [ ] 5b.4 Implement ## token pasting operator (concatenate adjacent tokens)
- [ ] 5b.5 Implement # stringification operator (convert argument to string literal)
- [ ] 5b.6 Implement __VA_ARGS__ substitution in variadic macros
- [ ] 5b.7 Implement macro expansion recursion prevention (mark macro as "expanding", skip self-references)
- [ ] 5b.8 Implement nested macro expansion (expand macros in expansion results, respecting recursion guard)
- [ ] 5b.9 Implement predefined macros: __FILE__ (current file path), __LINE__ (current line number)
- [ ] 5b.10 Implement NULL macro (expands to ((void *)0) or 0)
- [ ] 5b.11 Implement .i file output (write fully expanded source to disk)
- [ ] 5b.12 Write tests: expand simple object-like macros
- [ ] 5b.13 Write tests: expand function-like macros with arguments
- [ ] 5b.14 Write tests: verify ## token pasting (T##Result, fun_array_##T##_create, etc.)
- [ ] 5b.15 Write tests: verify # stringification
- [ ] 5b.16 Write tests: verify __VA_ARGS__ expansion
- [ ] 5b.17 Write tests: verify recursion prevention (macro referencing itself)
- [ ] 5b.18 Write tests: expand DEFINE_RESULT_TYPE(T) macro, diff against gcc -E
- [ ] 5b.19 Write tests: expand DEFINE_ARRAY_TYPE(T) macro, diff against gcc -E
- [ ] 5b.20 Write tests: expand DEFINE_HASHMAP_TYPE(K, V) macro, diff against gcc -E
- [ ] 5b.21 Write tests: full preprocessing pipeline (3a → 3b → 5a → 5b) on fundamental-cli files, diff against gcc -E

## Phase 6a: Symbol Collection

- [ ] 6a.1 Define Symbol struct (name, kind, declaration_node pointer, unresolved type info)
- [ ] 6a.2 Define SymbolKind enum (SYMBOL_FUNCTION, SYMBOL_VARIABLE, SYMBOL_TYPE, SYMBOL_ENUM_CONST)
- [ ] 6a.3 Implement symbol table structure: per-scope HashMap<String, Symbol>
- [ ] 6a.4 Implement global scope initialization
- [ ] 6a.5 Implement first pass over AST: collect function prototypes and definitions
- [ ] 6a.6 Implement first pass: collect global variable declarations
- [ ] 6a.7 Implement first pass: collect struct/union definitions (name → AST node)
- [ ] 6a.8 Implement first pass: collect enum definitions (name → AST node)
- [ ] 6a.9 Implement first pass: collect typedefs (name → target type specifier)
- [ ] 6a.10 Handle forward declarations (struct pointers before struct definition)
- [ ] 6a.11 Detect duplicate symbol errors at file scope
- [ ] 6a.12 Implement .sym serialization (write symbol table to binary file)
- [ ] 6a.13 Implement .sym deserialization
- [ ] 6a.14 Write tests: collect symbols from fundamental-cli source files, verify completeness
- [ ] 6a.15 Write tests: verify forward declaration handling

## Phase 6b: Type Resolution

- [ ] 6b.1 Define Type struct (TypeKind + union for pointer/array/record/enum/function)
- [ ] 6b.2 Define TypeKind enum (all primitive types, TYPE_POINTER, TYPE_ARRAY, TYPE_STRUCT, etc.)
- [ ] 6b.3 Implement primitive type constructors (void, char, int, float, double, all stdint types)
- [ ] 6b.4 Implement pointer type constructor (pointee → *Type)
- [ ] 6b.5 Implement array type constructor (element → *Type, count)
- [ ] 6b.6 Implement function type constructor (return_type, parameter list)
- [ ] 6b.7 Implement typedef chain resolution (walk typedef → typedef → concrete type)
- [ ] 6b.8 Implement struct layout computation (field offsets, alignment, padding, total size)
- [ ] 6b.9 Implement union layout computation (all fields at offset 0, size = max field size)
- [ ] 6b.10 Implement enum value assignment (explicit values + auto-increment)
- [ ] 6b.11 Build complete type graph: link all Type nodes via pointer/array/record references
- [ ] 6b.12 Detect circular typedef errors
- [ ] 6b.13 Detect incomplete struct errors (used by value but never defined)
- [ ] 6b.14 Implement .types serialization (write type graph to binary file)
- [ ] 6b.15 Implement .types deserialization
- [ ] 6b.16 Write tests: resolve all types in fundamental-cli, verify struct sizes match gcc
- [ ] 6b.17 Write tests: verify typedef chains resolve correctly

## Phase 6c: Name Resolution

- [ ] 6c.1 Implement scope stack (Array of scope levels, push on '{', pop on '}')
- [ ] 6c.2 Implement scope lookup (walk from innermost to outermost scope)
- [ ] 6c.3 Implement function scope creation (parameters become local symbols)
- [ ] 6c.4 Implement block scope creation (local variable declarations)
- [ ] 6c.5 Walk function bodies: resolve each AST_IDENT_EXPR to its Symbol entry
- [ ] 6c.6 Tag each AST_IDENT_EXPR node with .symbol pointer
- [ ] 6c.7 Handle variable shadowing (inner scope hides outer scope)
- [ ] 6c.8 Report undeclared identifier errors with file/line/col context
- [ ] 6c.9 Report unused variable warnings (optional, low priority)
- [ ] 6c.10 Implement .resolved serialization (write resolved AST to binary file)
- [ ] 6c.11 Implement .resolved deserialization
- [ ] 6c.12 Write tests: resolve names in fundamental-cli source files, verify no undeclared errors
- [ ] 6c.13 Write tests: verify variable shadowing works correctly

## Phase 6d: Type Checking

- [ ] 6d.1 Implement expression type computation (walk expression tree, compute result type per node)
- [ ] 6d.2 Implement binary expression type rules (arithmetic: int+int→int, pointer+int→pointer, etc.)
- [ ] 6d.3 Implement comparison expression types (always int/bool result)
- [ ] 6d.4 Implement unary expression type rules (dereference: *ptr→pointee, address-of: &x→pointer)
- [ ] 6d.5 Implement implicit integer promotions (char→int, int8→int, etc.)
- [ ] 6d.6 Implement implicit pointer conversions (void* ↔ any pointer, NULL → any pointer)
- [ ] 6d.7 Insert AST_IMPLICIT_CAST nodes where implicit conversions are needed
- [ ] 6d.8 Implement assignment type checking (RHS must be compatible with LHS type)
- [ ] 6d.9 Implement function call argument type checking (each arg vs parameter type)
- [ ] 6d.10 Implement return statement type checking (return value vs function return type)
- [ ] 6d.11 Implement struct/union member access type checking (. and ->)
- [ ] 6d.12 Implement array index type checking (index must be integer type)
- [ ] 6d.13 Implement sizeof evaluation (compile-time constant from Type.size_bytes)
- [ ] 6d.14 Implement cast expression type checking (explicit casts between compatible types)
- [ ] 6d.15 Implement designated initializer type checking (field names vs struct definition)
- [ ] 6d.16 Implement compound literal type checking
- [ ] 6d.17 Tag each expression AST node with .result_type: *Type
- [ ] 6d.18 Implement error reporting with file, line, column, and "expected X got Y" messages
- [ ] 6d.19 Implement .typed serialization (write fully typed AST to binary file)
- [ ] 6d.20 Implement .typed deserialization
- [ ] 6d.21 Write tests: type-check fundamental-cli source files, verify no type errors
- [ ] 6d.22 Write tests: verify implicit cast insertion (int→int64_t, etc.)
- [ ] 6d.23 Write tests: verify type error detection (mismatched types, wrong arg count, etc.)

## Phase 7: Code Generator — Expressions + Functions

- [ ] 7.1 Implement assembly output buffer (growable string for .s file content)
- [ ] 7.2 Implement label generation (unique labels for jumps and data)
- [ ] 7.3 Implement string literal emission (.rodata section)
- [ ] 7.4 Implement global variable emission (.data and .bss sections)
- [ ] 7.5 Implement function prologue (stack frame setup, callee-saved registers)
- [ ] 7.6 Implement function epilogue (stack frame teardown, return)
- [ ] 7.7 Implement Win64 ABI: register parameter passing (rcx, rdx, r8, r9 + shadow space)
- [ ] 7.8 Implement System V ABI: register parameter passing (rdi, rsi, rdx, rcx, r8, r9)
- [ ] 7.9 Implement local variable stack allocation (offsets from rbp)
- [ ] 7.10 Implement integer arithmetic codegen (add, sub, imul, idiv)
- [ ] 7.11 Implement comparison codegen (cmp + setcc)
- [ ] 7.12 Implement logical operators codegen (short-circuit evaluation)
- [ ] 7.13 Implement bitwise operators codegen (and, or, xor, shl, shr)
- [ ] 7.14 Implement unary operators codegen (negate, bitwise not, logical not, address-of, dereference)
- [ ] 7.15 Implement function call codegen (argument passing per ABI, call instruction, return value)
- [ ] 7.16 Implement assignment codegen (simple and compound assignments)
- [ ] 7.17 Implement pointer arithmetic codegen (scaled by pointee size)
- [ ] 7.18 Implement array indexing codegen (base + index * element_size)
- [ ] 7.19 Implement struct member access codegen (base + field offset)
- [ ] 7.20 Implement cast codegen (integer widening/narrowing, pointer casts)
- [ ] 7.21 Implement sizeof evaluation (compile-time constant)
- [ ] 7.22 Write codegen test: compile main() returning 42, assemble, link, run, verify exit code
- [ ] 7.23 Write codegen test: compile function with arithmetic, verify result
- [ ] 7.24 Write codegen test: compile function with pointer operations, verify result

## Phase 8: Code Generator — Control Flow + Structs

- [ ] 8.1 Implement if/else codegen (cmp + conditional jump + labels)
- [ ] 8.2 Implement for loop codegen (init + condition + increment + body + jump)
- [ ] 8.3 Implement while loop codegen (condition + body + jump)
- [ ] 8.4 Implement do-while loop codegen (body + condition + jump)
- [ ] 8.5 Implement break codegen (jump to loop/switch exit label)
- [ ] 8.6 Implement continue codegen (jump to loop continue label)
- [ ] 8.7 Implement switch/case codegen (comparison chain or jump table)
- [ ] 8.8 Implement return statement codegen (move to return register + epilogue)
- [ ] 8.9 Implement struct passing by value (copy to stack)
- [ ] 8.10 Implement struct return by value (caller-allocated space or register)
- [ ] 8.11 Implement compound literal codegen (allocate on stack, initialize fields)
- [ ] 8.12 Implement designated initializer codegen (field offset calculation + store)
- [ ] 8.13 Implement static local variable codegen (.data section with mangled name)
- [ ] 8.14 Implement ternary expression codegen (conditional jump pattern)
- [ ] 8.15 Implement increment/decrement codegen (pre and post)
- [ ] 8.16 Write codegen tests: compile programs with control flow, verify behavior
- [ ] 8.17 Write codegen tests: compile programs with structs, verify field access and layout
- [ ] 8.18 Write codegen tests: compile programs with switch statements, verify all cases

## Phase 9: Integration with fun build

- [ ] 9.1 Create cmd_compile.c / cmd_compile.h (fun compile command)
- [ ] 9.2 Implement compile command flag parsing: --norm, --inc, --cond, -E, --tokens, --lex, --ast, --symbols, --types, --resolved, --typed, -S, -c, -I, -o
- [ ] 9.3 Implement pipeline driver: each flag stops at the corresponding stage and writes output
- [ ] 9.4 Wire full pipeline: normalize → include → conditional → expand → tokenizer → lexer → parser → symcollect → typeresolve → nameresolve → typecheck → codegen
- [ ] 9.5 Register compile command in main.c
- [ ] 9.6 Create cmd_inspect.c / cmd_inspect.h (fun inspect-tokens, inspect-lex, inspect-ast, inspect-sym, inspect-types)
- [ ] 9.7 Implement inspect commands: deserialize binary format, print human-readable text
- [ ] 9.8 Register inspect commands in main.c
- [ ] 9.9 Implement fun build --compiler=funcc flag
- [ ] 9.10 Modify build generator to use funcc pipeline instead of gcc when flag is set
- [ ] 9.11 Implement multi-file compilation (compile each .c to .s, then assemble and link all)
- [ ] 9.12 Implement include path (-I) forwarding for vendor/fundamental/include
- [ ] 9.13 Implement assembler invocation (GNU as on the .s files)
- [ ] 9.14 Implement linker invocation (ld/link with platform-appropriate flags)
- [ ] 9.15 Write integration test: fun compile with each pipeline flag on a test file
- [ ] 9.16 Write integration test: fun build --compiler=funcc on a minimal project
- [ ] 9.17 Write integration test: fun build --compiler=funcc on fundamental-cli itself

## Phase 10: Validation

- [ ] 10.1 Compile fundamental-cli with funcc, compare binary behavior against gcc-compiled version
- [ ] 10.2 Run fundamental-cli test suite against funcc-compiled binary
- [ ] 10.3 Run fundamental-cli smoke test against funcc-compiled binary
- [ ] 10.4 Compile fundamental library test suites with funcc
- [ ] 10.5 Test on Windows (Win64 ABI)
- [ ] 10.6 Test on Linux (System V ABI)
- [ ] 10.7 Stress test: compile all source files, compare assembly output for correctness
- [ ] 10.8 Document known limitations and unsupported C features
- [ ] 10.9 Document funcc usage in CLAUDE.md and README
