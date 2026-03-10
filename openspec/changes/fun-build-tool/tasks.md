## 1. Platform Detection

- [ ] 1.1 Create src/fun/platform.c/h module
- [ ] 1.2 Implement Windows detection
- [ ] 1.3 Implement Linux detection
- [ ] 1.4 Implement macOS detection
- [ ] 1.5 Add platform string formatting (windows-amd64, etc.)

## 2. Build Script Detection

- [ ] 2.1 Create src/build/detector.c/h module
- [ ] 2.2 Implement check for build-windows-amd64.bat
- [ ] 2.3 Implement check for build-linux-amd64.sh
- [ ] 2.4 Return build script status (exists/missing)

## 3. Build Script Generation

- [ ] 3.1 Create src/build/generator.c/h module
- [ ] 3.2 Implement src/**/*.c file scanning
- [ ] 3.3 Create build-windows-amd64.bat template
- [ ] 3.4 Create build-linux-amd64.sh template
- [ ] 3.5 Generate Windows build script with source files
- [ ] 3.6 Generate Linux build script with source files
- [ ] 3.7 Include vendor/fundamental paths in generated scripts

## 4. Build Execution

- [ ] 5.1 Create src/build/executor.c/h module
- [ ] 5.2 Implement script execution for Windows
- [ ] 5.3 Implement script execution for Linux
- [ ] 5.4 Capture and forward exit codes
- [ ] 5.5 Handle execution errors

## 5. Command Implementation

- [ ] 5.1 Create src/commands/cmd_build.c/h
- [ ] 5.2 Implement --verbose flag
- [ ] 5.3 Implement --release flag (add -O3 -DNDEBUG)
- [ ] 5.4 Implement --clean flag (run fun clean first)
- [ ] 5.5 Create src/commands/cmd_clean.c/h
- [ ] 5.6 Implement artifact removal logic
- [ ] 5.7 Register commands in CLI router

## 6. fun.ini Integration

- [ ] 6.1 Parse [build] section from fun.ini
- [ ] 6.2 Read custom flags from config
- [ ] 6.3 Read entry point from config

## 7. Testing and Validation

- [ ] 7.1 Test fun build with existing scripts
- [ ] 7.2 Test fun build without scripts (generation)
- [ ] 7.3 Test fun build --verbose
- [ ] 7.4 Test fun build --release
- [ ] 7.5 Test fun clean
- [ ] 7.6 Test generated project builds successfully
