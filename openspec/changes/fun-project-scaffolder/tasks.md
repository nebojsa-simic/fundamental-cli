## 1. Setup and Infrastructure

- [ ] 1.1 Create directory structure for templates in src/
- [ ] 1.2 Add embedded template strings to source code
- [ ] 1.3 Create cmd_init.c/h command handler files
- [ ] 1.4 Register init command in CLI router

## 2. Template Implementation

- [ ] 2.1 Embed main.c template as C string
- [ ] 2.2 Embed cli.c template as C string
- [ ] 2.3 Embed cli.h template as C string
- [ ] 2.4 Embed cmd_version.c/h templates as C strings
- [ ] 2.5 Embed build-windows-amd64.bat template as C string
- [ ] 2.6 Embed build-linux-amd64.sh template as C string
- [ ] 2.7 Embed fun.ini template as C string
- [ ] 2.8 Embed README.md template as C string

## 3. Project Initialization Logic

- [ ] 3.1 Implement directory creation for project structure
- [ ] 3.2 Implement file generation from embedded templates
- [ ] 3.3 Implement project name validation and directory check
- [ ] 3.4 Handle existing directory error case

## 4. Fundamental Library Copy

- [ ] 4.1 Implement local copy from ../fundamental
- [ ] 4.2 Implement git clone fallback
- [ ] 4.3 Implement .git directory removal after clone
- [ ] 4.4 Add error handling for copy failures

## 5. Skill Scaffolding

- [ ] 5.1 Copy fundamental-expert/SKILL.md from vendor
- [ ] 5.2 Create .opencode/skills/ directory structure
- [ ] 5.3 Handle missing skill file gracefully

## 6. Arch Scaffolding

- [ ] 6.1 Create arch/startup/windows-amd64/windows.c template
- [ ] 6.2 Create arch/startup/linux-amd64/linux.c template
- [ ] 6.3 Generate appropriate arch file based on platform

## 7. Testing and Validation

- [ ] 7.1 Test fun init with project name
- [ ] 7.2 Test fun init in current directory
- [ ] 7.3 Test fun init with existing directory (error case)
- [ ] 7.4 Test generated project builds successfully
- [ ] 7.5 Verify all files are created correctly
