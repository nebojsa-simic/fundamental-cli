## 1. Test Discovery

- [x] 1.1 Create src/test/discovery.c/h module
- [x] 1.2 Implement tests/ directory scanning
- [x] 1.3 Filter for directories with test.c
- [x] 1.4 Return list of test modules
- [x] 1.5 Handle missing tests/ directory

## 2. Test Build Scaffolding

- [x] 2.1 Create src/test/scaffolder.c/h module
- [x] 2.2 Check for existing build scripts per test
- [x] 2.3 Generate build-windows-amd64.bat for tests
- [x] 2.4 Generate build-linux-amd64.sh for tests
- [x] 2.5 Include appropriate fundamental source files

## 3. Module-to-Source Mapping

- [x] 3.1 Create mapping data structure
- [x] 3.2 Add string module mapping (src/string/*.c)
- [x] 3.3 Add memory module mapping (arch/memory/*/memory.c)
- [x] 3.4 Add console module mapping
- [x] 3.5 Add async module mapping
- [x] 3.6 Add filesystem module mapping
- [x] 3.7 Create lookup function for module sources

## 4. Test Execution

- [x] 4.1 Create src/test/runner.c/h module
- [x] 4.2 Implement test build execution
- [x] 4.3 Implement test binary execution
- [x] 4.4 Capture exit codes
- [x] 4.5 Handle build failures
- [x] 4.6 Handle execution failures

## 5. Test Reporting

- [x] 5.1 Create src/test/reporter.c/h module
- [x] 5.2 Implement colored output (green ✓, red ✗)
- [x] 5.3 Implement pass/fail counting
- [x] 5.4 Implement summary output (X passed, Y failed)
- [x] 5.5 Set exit code based on failures

## 6. Command Implementation

- [x] 6.1 Create src/commands/cmd_test.c/h
- [x] 6.2 Implement --verbose flag
- [x] 6.3 Implement --filter <pattern> flag
- [x] 6.4 Implement --list flag
- [x] 6.5 Implement glob pattern matching for filter
- [x] 6.6 Register command in CLI router

## 7. Test Scaffolding Command

- [x] 7.1 Create src/commands/cmd_test_add.c/h
- [x] 7.2 Create test.c template
- [x] 7.3 Create test build script templates
- [x] 7.4 Implement tests/<module>/ directory creation
- [x] 7.5 Generate test file with module-specific includes
- [x] 7.6 Generate build scripts using module mapping
- [x] 7.7 Register command in CLI router

## 8. Testing and Validation

- [x] 8.1 Test fun test with sample tests
- [x] 8.2 Test fun test --filter
- [x] 8.3 Test fun test --list
- [x] 8.4 Test fun test add string
- [x] 8.5 Verify generated test builds and runs
- [x] 8.6 Test with no tests (graceful handling)
