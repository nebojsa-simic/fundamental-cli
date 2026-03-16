## Phase 1: Lexer + Token Types

- [ ] 1.1 Define TokenType enum with all keyword, literal, operator, and punctuation types
- [ ] 1.2 Define Token struct (type, value pointer, value length, line, col)
- [ ] 1.3 Implement lexer initialization (load source into memory buffer)
- [ ] 1.4 Implement single-character operator/punctuation tokenization
- [ ] 1.5 Implement multi-character operator tokenization (==, !=, <=, >=, &&, ||, <<, >>, ->, +=, etc.)
- [ ] 1.6 Implement identifier and keyword tokenization (with keyword lookup)
- [ ] 1.7 Implement integer literal tokenization (decimal, hex 0x, octal 0)
- [ ] 1.8 Implement float literal tokenization
- [ ] 1.9 Implement character literal tokenization (including escape sequences: \n, \t, \0, \\, \', \")
- [ ] 1.10 Implement string literal tokenization (including escape sequences)
- [ ] 1.11 Implement comment skipping (// line comments and /* block comments */)
- [ ] 1.12 Implement whitespace handling with line/column tracking
- [ ] 1.13 Implement ellipsis (...) tokenization
- [ ] 1.14 Write lexer tests: tokenize hand-written C fragments, verify token types and values
- [ ] 1.15 Write lexer tests: tokenize actual fundamental-cli source files (post gcc -E), verify no errors

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

## Phase 3: Preprocessor — Basic

- [ ] 3.1 Implement file reader (load source file into memory buffer using fun_read_file_in_memory)
- [ ] 3.2 Implement #include "file" resolution (search relative to current file, then include paths)
- [ ] 3.3 Implement #include <file> resolution (search system include paths only)
- [ ] 3.4 Implement include guard tracking (#ifndef GUARD / #define GUARD / #endif)
- [ ] 3.5 Implement #pragma once support
- [ ] 3.6 Implement object-like #define expansion (simple text substitution)
- [ ] 3.7 Implement #undef
- [ ] 3.8 Implement output buffer (growable char array for preprocessed text)
- [ ] 3.9 Write preprocessor tests: preprocess simple files with #include and #define, diff against gcc -E
- [ ] 3.10 Test with actual fundamental-cli header files (include chains)

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

## Phase 5: Preprocessor — Full

- [ ] 5.1 Implement function-like macro definition (#define FOO(x, y) ...)
- [ ] 5.2 Implement function-like macro expansion (argument substitution)
- [ ] 5.3 Implement ## token pasting operator
- [ ] 5.4 Implement __VA_ARGS__ support in variadic macros
- [ ] 5.5 Implement # stringification operator
- [ ] 5.6 Implement #ifdef / #ifndef conditional compilation
- [ ] 5.7 Implement #if / #elif with constant expression evaluation
- [ ] 5.8 Implement defined() operator in #if expressions
- [ ] 5.9 Implement #else / #endif
- [ ] 5.10 Implement nested conditional compilation
- [ ] 5.11 Implement #pragma GCC diagnostic push/pop/ignored (pass through or ignore)
- [ ] 5.12 Implement predefined macros: __FILE__, __LINE__, NULL
- [ ] 5.13 Implement macro expansion recursion prevention (no self-referential expansion)
- [ ] 5.14 Write preprocessor tests: expand DEFINE_RESULT_TYPE(T) macro, verify output
- [ ] 5.15 Write preprocessor tests: expand DEFINE_ARRAY_TYPE(T) macro, verify output
- [ ] 5.16 Write preprocessor tests: expand DEFINE_HASHMAP_TYPE(K, V) macro, verify output
- [ ] 5.17 Test full preprocessing of fundamental-cli source files, diff against gcc -E output

## Phase 6: Type System + Semantic Analysis

- [ ] 6.1 Define Type struct (TypeKind + union for pointer/array/struct/union/enum/function details)
- [ ] 6.2 Implement primitive type constructors (void, char, int, float, double, stdint types)
- [ ] 6.3 Implement pointer type constructor
- [ ] 6.4 Implement array type constructor
- [ ] 6.5 Implement struct/union type layout (field offsets, alignment, padding, total size)
- [ ] 6.6 Implement enum type (enumerator values)
- [ ] 6.7 Implement function type (return type + parameter list)
- [ ] 6.8 Implement typedef resolution (name → underlying type)
- [ ] 6.9 Implement symbol table (per-scope HashMap<String, Symbol>)
- [ ] 6.10 Implement scope stack (push on '{', pop on '}', lookup walks stack)
- [ ] 6.11 Implement name resolution pass (resolve identifiers to declarations)
- [ ] 6.12 Implement type checking for expressions (binary ops, unary ops, function calls, member access)
- [ ] 6.13 Implement implicit integer promotion and conversion rules
- [ ] 6.14 Implement pointer/integer conversion checking
- [ ] 6.15 Implement assignment type compatibility checking
- [ ] 6.16 Implement function call argument type checking
- [ ] 6.17 Implement struct member lookup (name → field type + offset)
- [ ] 6.18 Implement type checking for initializers (designated and positional)
- [ ] 6.19 Implement error reporting with file, line, column context
- [ ] 6.20 Write type system tests: type-check fundamental-cli source files, verify no errors

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
- [ ] 9.2 Implement compile command argument parsing (input file, -I include paths, -o output, -E preprocess-only, -S asm-only)
- [ ] 9.3 Wire compile command: preprocessor → lexer → parser → sema → codegen → write .s file
- [ ] 9.4 Register compile command in main.c
- [ ] 9.5 Implement fun build --compiler=funcc flag
- [ ] 9.6 Modify build generator to emit funcc invocations instead of gcc when flag is set
- [ ] 9.7 Implement multi-file compilation (compile each .c to .s, then assemble and link all)
- [ ] 9.8 Implement include path (-I) forwarding for vendor/fundamental/include
- [ ] 9.9 Implement assembler invocation (as/nasm on the .s files)
- [ ] 9.10 Implement linker invocation (ld/link with platform-appropriate flags)
- [ ] 9.11 Write integration test: fun build --compiler=funcc on a minimal project
- [ ] 9.12 Write integration test: fun build --compiler=funcc on fundamental-cli itself

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
