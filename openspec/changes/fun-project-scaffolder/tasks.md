## 1. Setup and Infrastructure

- [x] 1.1 Create directory structure for templates in src/
- [x] 1.2 Add embedded template strings to source code
- [x] 1.3 Create cmd_init.c/h command handler files
- [x] 1.4 Register init command in CLI router

## 2. Template Implementation

- [x] 2.1 Embed main.c template as C string
- [x] 2.2 Embed cli.c template as C string
- [x] 2.3 Embed cli.h template as C string
- [x] 2.4 Embed cmd_version.c/h templates as C strings
- [x] 2.5 Embed build-windows-amd64.bat template as C string
- [x] 2.6 Embed build-linux-amd64.sh template as C string
- [x] 2.7 Embed fun.ini template as C string
- [x] 2.8 Embed README.md template as C string
- [x] 2.9 Embed fundamental-expert/SKILL.md template as C string

## 3. Project Initialization Logic

- [ ] 3.1 Implement directory creation for src/, commands/, vendor/, .opencode/skills/
- [ ] 3.2 Implement file generation from embedded templates
- [ ] 3.3 Implement current directory validation (must be empty or not exist)
- [ ] 3.4 Handle existing files error case

## 4. Fundamental Library Copy (Selective)

- [ ] 4.1 Implement copy of arch/ folder only
- [ ] 4.2 Implement copy of include/ folder only
- [ ] 4.3 Implement copy of src/ folder only
- [ ] 4.4 Exclude tests/, openspec/, node_modules/ and other non-essential folders
- [ ] 4.5 Implement local copy from ../fundamental (selective folders)
- [ ] 4.6 Implement git clone fallback (then copy selective folders)
- [ ] 4.7 Implement .git directory removal after clone
- [ ] 4.8 Add error handling for copy failures

## 5. Skill Scaffolding (Embedded Template)

- [ ] 5.1 Embed fundamental-expert/SKILL.md as C string template
- [ ] 5.2 Create .opencode/skills/fundamental-expert/ directory
- [ ] 5.3 Write SKILL.md from embedded template (not copied from vendor)
- [ ] 5.4 Handle template write errors

## 6. Arch Scaffolding

- [ ] 6.1 Create arch/startup/windows-amd64/windows.c template
- [ ] 6.2 Create arch/startup/linux-amd64/linux.c template
- [ ] 6.3 Generate appropriate arch file based on platform

## 7. Testing and Validation

- [ ] 7.1 Test fun init in empty directory
- [ ] 7.2 Test fun init in non-empty directory (error case)
- [ ] 7.3 Test generated project builds successfully
- [ ] 7.4 Verify only arch/, include/, src/ are copied to vendor/fundamental/
- [ ] 7.5 Verify tests/, openspec/, node_modules/ are NOT copied
- [ ] 7.6 Verify skill is generated from embedded template
- [ ] 7.7 Verify all essential files are created correctly
- [ ] 7.8 Test workflow: mkdir project && cd project && fun init
