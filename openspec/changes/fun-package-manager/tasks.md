## 1. Dependency Parsing

- [ ] 1.1 Create src/package/parser.c/h module
- [ ] 1.2 Implement fundamental shorthand parsing
- [ ] 1.3 Implement user/repo GitHub shorthand parsing
- [ ] 1.4 Implement HTTPS URL parsing
- [ ] 1.5 Implement SSH URL parsing
- [ ] 1.6 Implement local path parsing
- [ ] 1.7 Return structured dependency info

## 2. Git Resolution

- [ ] 2.1 Create src/package/git.c/h module
- [ ] 2.2 Implement git clone command execution
- [ ] 2.3 Implement git checkout for versions/tags
- [ ] 2.4 Implement .git directory removal
- [ ] 2.5 Handle git errors (repo not found, network)
- [ ] 2.6 Clean up partial clones on failure

## 3. Local Resolution

- [ ] 3.1 Create src/package/local.c/h module
- [ ] 3.2 Implement path validation
- [ ] 3.3 Implement directory copy operation
- [ ] 3.4 Handle missing path error
- [ ] 3.5 Handle copy errors

## 4. Vendor Management

- [ ] 4.1 Create src/package/vendor.c/h module
- [ ] 4.2 Implement vendor/ directory creation
- [ ] 4.3 Implement copy to vendor/<name>
- [ ] 4.4 Ensure copy (not symlink) strategy
- [ ] 4.5 Implement vendor directory cleanup

## 5. fun.ini Dependencies Management

- [ ] 5.1 Create src/package/ini.c/h module
- [ ] 5.2 Parse [dependencies] section
- [ ] 5.3 Add dependency to [dependencies]
- [ ] 5.4 Remove dependency from [dependencies]
- [ ] 5.5 Handle missing fun.ini gracefully

## 6. Command Implementation - Add

- [ ] 6.1 Create src/commands/cmd_add.c/h
- [ ] 6.2 Parse dependency argument
- [ ] 6.3 Resolve dependency (git or local)
- [ ] 6.4 Copy to vendor/
- [ ] 6.5 Update fun.ini
- [ ] 6.6 Register command in CLI router

## 7. Command Implementation - Install

- [ ] 7.1 Create src/commands/cmd_install.c/h
- [ ] 7.2 Parse fun.ini dependencies
- [ ] 7.3 Install each dependency
- [ ] 7.4 Report installation progress
- [ ] 7.5 Register command in CLI router

## 8. Command Implementation - Remove

- [ ] 8.1 Create src/commands/cmd_remove.c/h
- [ ] 8.2 Remove from fun.ini
- [ ] 8.3 Remove from vendor/
- [ ] 8.4 Handle missing dependency warning
- [ ] 8.5 Register command in CLI router

## 9. Command Implementation - List

- [ ] 9.1 Create src/commands/cmd_list.c/h
- [ ] 9.2 List dependencies from fun.ini
- [ ] 9.3 List installed packages from vendor/
- [ ] 9.4 Show warnings for mismatched deps
- [ ] 9.5 Register command in CLI router

## 10. Testing and Validation

- [ ] 10.1 Test fun add fundamental
- [ ] 10.2 Test fun add user/repo
- [ ] 10.3 Test fun add with version tag
- [ ] 10.4 Test fun add ../local-path
- [ ] 10.5 Test fun install
- [ ] 10.6 Test fun remove
- [ ] 10.7 Test fun list
