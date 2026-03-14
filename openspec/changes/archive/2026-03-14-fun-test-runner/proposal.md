## Why

Running tests across a fundamental library project requires manual execution of each test's build script and binary. The fundamental library's `run-tests-windows-amd64.bat` provides a pattern, but projects need an integrated test runner with discovery, filtering, reporting, and test scaffolding capabilities.

## What Changes

- **New**: `fun test` command to discover and execute all tests in `tests/` directory
- **New**: Test discovery by scanning `tests/*/` directories
- **New**: Automatic build script scaffolding per test module if missing
- **New**: Test execution with exit code collection and summary reporting
- **New**: Colored output (green ✓ for pass, red ✗ for fail)
- **New**: `fun test add <module>` to scaffold new test modules with build scripts and test file
- **New**: Test filtering with `--filter <pattern>` for glob matching
- **New**: Test listing with `--list` to show available tests without running
- **New**: Verbose mode with `--verbose` for detailed build and execution output

## Capabilities

### New Capabilities

- `test-discovery`: Recursive scanning of `tests/` directory to find test modules
- `test-build-scaffolding`: Generation of build scripts for test modules based on module type
- `test-execution`: Sequential or parallel execution of test binaries with exit code tracking
- `test-reporting`: Colored output, pass/fail counts, and summary statistics
- `test-scaffolding`: Creation of new test modules with template test files and build scripts
- `module-mapping`: Knowledge base mapping module names to required fundamental source files
- `test-filtering`: Glob pattern matching for selective test execution

### Modified Capabilities

<!-- No existing capabilities modified -->

## Impact

- **Code**: New modules in `src/test/discovery.c/h`, `src/test/runner.c/h`, `src/test/reporter.c/h`, `src/test/scaffolder.c/h`
- **Commands**: `src/commands/cmd_test.c/h`, `src/commands/cmd_test_add.c/h`
- **Templates**: Embedded test templates (test.c, build-windows.bat, build-linux.sh)
- **Mappings**: Module name → source file mapping for test build generation
- **Dependencies**: Uses fundamental's async process spawn for test execution
