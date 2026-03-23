## Phase 1a: Tokenizer (Pass 1 — language-agnostic)

- [x] 1a.1 Write tokenizer.grammar: define all token patterns (WORD, NUMBER, STRING, CHAR, symbols)
- [x] 1a.2 Define RawTokenType enum (RAW_WORD, RAW_NUMBER, RAW_STRING, RAW_CHAR, all symbols, RAW_EOF)
- [x] 1a.3 Define RawToken struct (type, offset, length, line, col)
- [x] 1a.4 Implement tokenizer initialization (accept source buffer + length, init cursor state)
- [x] 1a.5 Implement whitespace consumption with line/column tracking
- [x] 1a.6 Implement comment skipping (// line comments and /* block comments */)
- [x] 1a.7 Implement WORD tokenization: [a-zA-Z_][a-zA-Z0-9_]*
- [x] 1a.8 Implement NUMBER tokenization: decimal, hex (0x/0X), octal (0), float (with .)
- [x] 1a.9 Implement STRING tokenization: "..." (raw bytes, track escape sequences but don't resolve)
- [x] 1a.10 Implement CHAR tokenization: '...' (raw bytes, track escape sequences but don't resolve)
- [x] 1a.11 Implement single-character symbol tokenization (all punctuation and operators)
- [x] 1a.12 Implement multi-character symbol tokenization (==, !=, <=, >=, &&, ||, <<, >>, ->, +=, -=, etc.)
- [x] 1a.13 Implement ellipsis (...) tokenization
- [x] 1a.14 Implement tokenizer error reporting (unexpected character, unterminated string/char/comment)
- [x] 1a.15 Implement .tokens serialization (write Array<RawToken> to binary file)
- [x] 1a.16 Implement .tokens deserialization (read binary file to Array<RawToken>)
- [x] 1a.17 Write tokenizer tests: verify against tokenizer.grammar patterns
- [x] 1a.18 Write tokenizer tests: tokenize hand-written C fragments, verify token types and positions
- [x] 1a.19 Write tokenizer tests: tokenize actual fundamental-cli source files (post gcc -E), verify no errors

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

## Phase 7a: IR Generation (TypedAST → LLVM IR subset)

- [ ] 7a.1 Define in-memory IR data structures: IRModule, IRFunction, IRBasicBlock, IRInstruction
- [ ] 7a.2 Define IR type system: i8, i16, i32, i64, float, double, ptr, void, struct types, array types, function types
- [ ] 7a.3 Define IR value representation: virtual register (%0, %1, ...), constants, global references
- [ ] 7a.4 Implement IR text emitter (write LLVM IR subset to .ll text file)
- [ ] 7a.5 Implement function translation: TypedAST FunctionDecl → IR define with entry block
- [ ] 7a.6 Implement alloca generation: every local variable → alloca instruction at function entry
- [ ] 7a.7 Implement parameter handling: function params → alloca + store at entry
- [ ] 7a.8 Implement variable read: identifier → load from alloca
- [ ] 7a.9 Implement variable write: assignment → store to alloca
- [ ] 7a.10 Implement integer arithmetic: +, -, *, /, % → add, sub, mul, sdiv/udiv, srem/urem
- [ ] 7a.11 Implement bitwise operators: &, |, ^, ~, <<, >> → and, or, xor, shl, lshr/ashr
- [ ] 7a.12 Implement comparison operators: ==, !=, <, >, <=, >= → icmp with predicates
- [ ] 7a.13 Implement logical operators: &&, || → short-circuit with br (conditional) + basic blocks
- [ ] 7a.14 Implement unary operators: -, !, ~ → sub 0, icmp eq 0, xor -1
- [ ] 7a.15 Implement address-of (&) and dereference (*) → alloca address / load through ptr
- [ ] 7a.16 Implement function call: call instruction with arguments and return value
- [ ] 7a.17 Implement if/else → conditional br + basic blocks (then, else, merge)
- [ ] 7a.18 Implement for loop → basic blocks (init, cond, body, inc, exit) + br
- [ ] 7a.19 Implement while loop → basic blocks (cond, body, exit) + br
- [ ] 7a.20 Implement do-while loop → basic blocks (body, cond, exit) + br
- [ ] 7a.21 Implement break/continue → unconditional br to loop exit/continue block
- [ ] 7a.22 Implement switch/case → switch instruction with case labels + default
- [ ] 7a.23 Implement return → ret instruction
- [ ] 7a.24 Implement struct member access → getelementptr with struct field index
- [ ] 7a.25 Implement array indexing → getelementptr with index
- [ ] 7a.26 Implement pointer arithmetic → getelementptr scaled by pointee type
- [ ] 7a.27 Implement type casts → zext, sext, trunc, bitcast, ptrtoint, inttoptr
- [ ] 7a.28 Implement sizeof → compile-time constant (IR integer literal)
- [ ] 7a.29 Implement float arithmetic → fadd, fsub, fmul, fdiv
- [ ] 7a.30 Implement float comparisons → fcmp
- [ ] 7a.31 Implement string literal → global constant in IR (.rodata)
- [ ] 7a.32 Implement global variable → IR global definition (.data / .bss)
- [ ] 7a.33 Implement static local variable → IR internal global with mangled name
- [ ] 7a.34 Implement compound literal → alloca + getelementptr + store for each field
- [ ] 7a.35 Implement designated initializer → field-indexed getelementptr + store
- [ ] 7a.36 Implement compound assignment (+=, -=, etc.) → load + op + store sequence
- [ ] 7a.37 Implement pre/post increment/decrement → load + add/sub + store (+ sequence point)
- [ ] 7a.38 Implement ternary expression → conditional br + basic blocks + select pattern
- [ ] 7a.39 Implement comma operator → evaluate left (discard), evaluate right (keep)
- [ ] 7a.40 Write tests: translate minimal functions, verify .ll output matches expected IR
- [ ] 7a.41 Write tests: validate .ll output with llc (if available) as correctness oracle
- [ ] 7a.42 Write tests: translate functions with control flow, verify basic block structure

## Phase 7b: ABI Lowering

- [ ] 7b.1 Define ABI interface: struct with function pointers for arg passing, return, caller/callee-saved regs
- [ ] 7b.2 Implement Win64 ABI module: arg regs (rcx, rdx, r8, r9), shadow space, caller-saved set
- [ ] 7b.3 Implement System V ABI module: arg regs (rdi, rsi, rdx, rcx, r8, r9), red zone, caller-saved set
- [ ] 7b.4 Implement ABI dispatch: select Win64 or System V based on target platform
- [ ] 7b.5 Implement call lowering: rewrite call instructions → explicit arg register moves + call + result move
- [ ] 7b.6 Implement return lowering: rewrite ret → move to rax + epilogue sequence
- [ ] 7b.7 Implement stack argument passing: args beyond register count → push to stack per ABI
- [ ] 7b.8 Implement struct passing by value: small structs in regs, large structs via pointer per ABI
- [ ] 7b.9 Implement struct return by value: caller-allocated space or register per ABI
- [ ] 7b.10 Implement caller-saved register annotation: mark which regs are clobbered across calls
- [ ] 7b.11 Implement stack alignment enforcement: 16-byte alignment before call per ABI
- [ ] 7b.12 Implement Win64 shadow space insertion: reserve 32 bytes before call arguments
- [ ] 7b.13 Implement .abi.ll text output: write ABI-lowered IR to disk (still LLVM IR text form)
- [ ] 7b.14 Write tests: verify Win64 call lowering (arg registers, shadow space, alignment)
- [ ] 7b.15 Write tests: verify System V call lowering (arg registers, stack args, alignment)
- [ ] 7b.16 Write tests: verify struct passing for both ABIs
- [ ] 7b.17 Write tests: verify caller-saved register annotations

## Phase 7c: Register Allocation (Linear Scan)

- [ ] 7c.1 Define physical register set: rax, rbx, rcx, rdx, rsi, rdi, r8-r15 (minus rsp, rbp)
- [ ] 7c.2 Define live interval data structure: virtual register → [start, end] range
- [ ] 7c.3 Implement live interval computation: walk instructions, track first/last use of each virtual reg
- [ ] 7c.4 Implement interval sorting by start point
- [ ] 7c.5 Implement linear scan core: walk sorted intervals, assign physical registers from available pool
- [ ] 7c.6 Implement spill decision: when pool exhausted, spill interval ending furthest out
- [ ] 7c.7 Implement spill slot allocation: assign stack offsets [rbp - N] for spilled registers
- [ ] 7c.8 Implement spill code insertion: load before use, store after def for spilled intervals
- [ ] 7c.9 Implement pre-colored register handling: ABI-required registers (rax for return, arg regs)
- [ ] 7c.10 Implement callee-saved register tracking: record which callee-saved regs are used (for prologue/epilogue)
- [ ] 7c.11 Implement frame size calculation: sum of spill slots + local allocas + alignment padding
- [ ] 7c.12 Implement AllocatedIR data structure: all virtual regs replaced with physical regs or spill offsets
- [ ] 7c.13 Implement .alloc binary serialization (write allocated IR to disk)
- [ ] 7c.14 Implement .alloc binary deserialization
- [ ] 7c.15 Write tests: allocate registers for simple functions, verify no conflicts
- [ ] 7c.16 Write tests: verify spilling occurs when register pressure exceeds available pool
- [ ] 7c.17 Write tests: verify callee-saved registers are tracked correctly
- [ ] 7c.18 Write tests: verify frame size calculation matches expected layout

## Phase 7d: Assembly Emission

- [ ] 7d.1 Implement assembly output buffer (growable string via DEFINE_ARRAY_TYPE(char))
- [ ] 7d.2 Implement section management: .text, .rodata, .data, .bss switching
- [ ] 7d.3 Implement label generation: unique labels for basic blocks, jumps, data
- [ ] 7d.4 Implement function emission: .globl directive, label, prologue, body, epilogue
- [ ] 7d.5 Implement prologue emission: pushq %rbp, movq %rsp %rbp, subq $frame_size %rsp, save callee-saved regs
- [ ] 7d.6 Implement epilogue emission: restore callee-saved regs, movq %rbp %rsp, popq %rbp, ret
- [ ] 7d.7 Implement instruction emission: translate each allocated IR instruction to AT&T syntax
- [ ] 7d.8 Implement memory operand formatting: offset(%reg), scale*index+base patterns
- [ ] 7d.9 Implement immediate operand formatting: $value
- [ ] 7d.10 Implement string literal emission: .section .rodata, .string directive
- [ ] 7d.11 Implement float constant emission: .section .rodata, .long/.quad for float/double bit patterns
- [ ] 7d.12 Implement global variable emission: .data/.bss section, .long/.quad/.byte with initializers
- [ ] 7d.13 Implement static variable emission: .local directive + data section
- [ ] 7d.14 Implement extern symbol directives
- [ ] 7d.15 Implement alignment directives: .align for functions, data, stack
- [ ] 7d.16 Implement .s file output: write final assembly text to disk
- [ ] 7d.17 Write tests: emit assembly for minimal function, assemble with GNU as, verify no errors
- [ ] 7d.18 Write tests: emit + assemble + link + run main() returning 42, verify exit code
- [ ] 7d.19 Write tests: emit + assemble + link + run function with arithmetic, verify result
- [ ] 7d.20 Write tests: emit + assemble + link + run function with control flow, verify result
- [ ] 7d.21 Write tests: emit + assemble + link + run function with struct access, verify result
- [ ] 7d.22 Write tests: emit + assemble + link + run function with pointer ops, verify result

## Phase 8: Integration with fun build

- [ ] 8.1 Create cmd_compile.c / cmd_compile.h (fun compile command)
- [ ] 8.2 Implement compile command flag parsing: --preprocessor-normalized, --preprocessor-includes, --preprocessor-conditionals, --preprocessor-macros, --tokens, --lexed, --ast-parsed, --ast-symbols, --ast-types, --ast-resolved, --ast-typed, --ir, --ir-abi, --ir-registers, --assembly, --object, -I, -o
- [ ] 8.3 Implement pipeline driver: each flag stops at the corresponding stage and writes output
- [ ] 8.4 Wire full pipeline: normalize → include → conditional → expand → tokenizer → lexer → parser → symcollect → typeresolve → nameresolve → typecheck → irgen → abi_lower → regalloc → asmemit
- [ ] 8.5 Register compile command in main.c
- [ ] 8.6 Create cmd_inspect.c / cmd_inspect.h (fun inspect-tokens, inspect-lex, inspect-ast, inspect-sym, inspect-types, inspect-alloc)
- [ ] 8.7 Implement inspect commands: deserialize binary format, print human-readable text
- [ ] 8.8 Register inspect commands in main.c
- [ ] 8.9 Implement fun build --compiler=funcc flag
- [ ] 8.10 Modify build generator to use funcc pipeline instead of gcc when flag is set
- [ ] 8.11 Implement multi-file compilation (compile each .c to .s, then assemble and link all)
- [ ] 8.12 Implement include path (-I) forwarding for vendor/fundamental/include
- [ ] 8.13 Implement assembler invocation (GNU as on the .s files)
- [ ] 8.14 Implement linker invocation (ld/link with platform-appropriate flags)
- [ ] 8.15 Write integration test: fun compile with each pipeline flag on a test file
- [ ] 8.16 Write integration test: fun build --compiler=funcc on a minimal project
- [ ] 8.17 Write integration test: fun build --compiler=funcc on fundamental-cli itself

## Phase 9: Validation

- [ ] 9.1 Compile fundamental-cli with funcc, compare binary behavior against gcc-compiled version
- [ ] 9.2 Run fundamental-cli test suite against funcc-compiled binary
- [ ] 9.3 Run fundamental-cli smoke test against funcc-compiled binary
- [ ] 9.4 Compile fundamental library test suites with funcc
- [ ] 9.5 Test on Windows (Win64 ABI)
- [ ] 9.6 Test on Linux (System V ABI)
- [ ] 9.7 Stress test: compile all source files, compare assembly output for correctness
- [ ] 9.8 Document known limitations and unsupported C features
- [ ] 9.9 Document funcc usage in CLAUDE.md and README
