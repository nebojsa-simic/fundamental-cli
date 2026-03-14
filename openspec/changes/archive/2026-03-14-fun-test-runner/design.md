## Context

Running tests requires manually executing each test's build script and binary. The fundamental library's `run-tests-windows-amd64.bat` provides a pattern, but projects need an integrated test runner with discovery, filtering, reporting, and scaffolding capabilities.

## Goals / Non-Goals

**Goals:**
- Discover tests by scanning tests/*/ directories
- Scaffold build scripts for test modules if missing
- Execute tests with colored pass/fail reporting
- Support --filter, --list, --verbose flags
- Scaffold new tests with fun test add <module>
- Map module names to required fundamental source files

**Non-Goals:**
- Custom test frameworks (each test is standalone)
- Parallel test execution (sequential for now)
- Test coverage reporting (can add later)

## Decisions

### Decision 1: Mimic fundamental's run-tests batch file pattern
**Rationale:** Proven pattern, familiar to fundamental developers, simple and effective.

**Alternatives considered:**
- Custom test framework: Adds dependency, complexity
- xUnit-style tests: Overkill for C projects

### Decision 2: Each test module is self-contained with own build script
**Rationale:** Isolation, can build/run tests independently, matches fundamental structure.

**Alternatives considered:**
- Single test binary: Harder to isolate failures
- Centralized build: More complex dependency management

### Decision 3: Module-to-source mapping for scaffolding
**Rationale:** Automates build script generation, users don't need to know fundamental file structure.

**Alternatives considered:**
- User specifies sources: More configuration burden
- Scan fundamental for dependencies: Complex, slow

### Decision 4: Colored output with ✓/✗ symbols
**Rationale:** Clear visual feedback, follows modern CLI conventions.

**Alternatives considered:**
- Plain text output: Less readable
- Custom symbols: May not render on all terminals

## Risks / Trade-offs

**[Sequential execution is slower]** → Mitigation: Can add parallel execution later, tests are usually fast

**[Module mapping requires maintenance]** → Mitigation: Centralized mapping table, update when fundamental changes

**[Colored output may not work on all terminals]** → Mituation: Detect terminal capabilities, fall back to plain text

## Open Questions

- Should fun test support timeout per test?
- Should failed tests continue or stop immediately?
