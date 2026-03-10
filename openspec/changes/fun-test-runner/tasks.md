## 1. Test Discovery

- [ ] 1.1 Create src/test/discovery.c/h module
- [ ] 1.2 Implement tests/ directory scanning
- [ ] 1.3 Filter for directories with test.c
- [ ] 1.4 Return list of test modules
- [ ] 1.5 Handle missing tests/ directory

## 2. Test Build Scaffolding

- [ ] 2.1 Create src/test/scaffolder.c/h module
- [ ] 2.2 Check for existing build scripts per test
- [ ] 2.3 Generate build-windows-amd64.bat for tests
- [ ] 2.4 Generate build-linux-amd64.sh for tests
- [ ] 2.5 Include appropriate fundamental source files

## 3. Module-to-Source Mapping

- [ ] 3.1 Create mapping data structure
- [ ] 3.2 Add string module mapping (src/string/*.c)
- [ ] 3.3 Add memory module mapping (arch/memory/*/memory.c)
- [ ] 3.4 Add console module mapping
- [ ] 3.5 Add async module mapping
- [ ] 3.6 Add filesystem module mapping
- [ ] 3.7 Create lookup function for module sources

## 4. Test Execution

- [ ] 4.1 Create src/test/runner.c/h module
- [ ] 4.2 Implement test build execution
- [ ] 4.3 Implement test binary execution
- [ ] 4.4 Capture exit codes
- [ ] 4.5 Handle build failures
- [ ] 4.6 Handle execution failures

## 5. Test Reporting

- [ ] 5.1 Create src/test/reporter.c/h module
- [ ] 5.2 Implement colored output (green ✓, red ✗)
- [ ] 5.3 Implement pass/fail counting
- [ ] 5.4 Implement summary output (X passed, Y failed)
- [ ] 5.5 Set exit code based on failures

## 6. Command Implementation

- [ ] 6.1 Create src/commands/cmd_test.c/h
- [ ] 6.2 Implement --verbose flag
- [ ] 6.3 Implement --filter <pattern> flag
- [ ] 6.4 Implement --list flag
- [ ] 6.5 Implement glob pattern matching for filter
- [ ] 6.6 Register command in CLI router

## 7. Test Scaffolding Command

- [ ] 7.1 Create src/commands/cmd_test_add.c/h
- [ ] 7.2 Create test.c template
- [ ] 7.3 Create test build script templates
- [ ] 7.4 Implement tests/<module>/ directory creation
- [ ] 7.5 Generate test file with module-specific includes
- [ ] 7.6 Generate build scripts using module mapping
- [ ] 7.7 Register command in CLI router

## 8. Testing and Validation

- [ ] 8.1 Test fun test with sample tests
- [ ] 8.2 Test fun test --filter
- [ ] 8.3 Test fun test --list
- [ ] 8.4 Test fun test add string
- [ ] 8.5 Verify generated test builds and runs
- [ ] 8.6 Test with no tests (graceful handling)
