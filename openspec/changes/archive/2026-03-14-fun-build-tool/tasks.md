## 1. Platform Detection

- [x] 1.1 Create src/fun/platform.c/h module
- [x] 1.2 Implement Windows detection
- [x] 1.3 Implement Linux detection
- [x] 1.4 Implement macOS detection
- [x] 1.5 Add platform string formatting (windows-amd64, etc.)

## 2. Build Script Detection

- [x] 2.1 Create src/build/detector.c/h module
- [x] 2.2 Implement check for build-windows-amd64.bat
- [x] 2.3 Implement check for build-linux-amd64.sh
- [x] 2.4 Return build script status (exists/missing)

## 3. Build Script Generation

- [x] 3.1 Create src/build/generator.c/h module
- [x] 3.2 Implement src/**/*.c file scanning
- [x] 3.3 Create build-windows-amd64.bat template
- [x] 3.4 Create build-linux-amd64.sh template
- [x] 3.5 Generate Windows build script with source files
- [x] 3.6 Generate Linux build script with source files
- [x] 3.7 Include vendor/fundamental paths in generated scripts

## 4. Build Execution

- [x] 5.1 Create src/build/executor.c/h module
- [x] 5.2 Implement script execution for Windows
- [x] 5.3 Implement script execution for Linux
- [x] 5.4 Capture and forward exit codes
- [x] 5.5 Handle execution errors

## 5. Command Implementation

- [x] 5.1 Create src/commands/cmd_build.c/h
- [x] 5.2 Implement --verbose flag
- [x] 5.3 Implement --release flag (add -O3 -DNDEBUG)
- [x] 5.4 Implement --clean flag (run fun clean first)
- [x] 5.5 Create src/commands/cmd_clean.c/h
- [x] 5.6 Implement artifact removal logic
- [x] 5.7 Register commands in CLI router

## 6. fun.ini Integration

- [x] 6.1 Parse [build] section from fun.ini
- [x] 6.2 Read custom flags from config
- [x] 6.3 Read entry point from config

## 7. Testing and Validation

- [x] 7.1 Test fun build with existing scripts
- [x] 7.2 Test fun build without scripts (generation)
- [x] 7.3 Test fun build --verbose
- [x] 7.4 Test fun build --release
- [x] 7.5 Test fun clean
- [x] 7.6 Test generated project builds successfully
