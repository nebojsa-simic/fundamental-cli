## ADDED Requirements

### Requirement: System scaffolds minimal arch structure
The system SHALL create minimal `arch/startup/<platform>/` directories following fundamental principles.

#### Scenario: Create Windows startup
- **WHEN** project is scaffolded on Windows
- **THEN** `arch/startup/windows-amd64/windows.c` is created

#### Scenario: Create Linux startup
- **WHEN** project is scaffolded on Linux
- **THEN** `arch/startup/linux-amd64/linux.c` is created

### Requirement: Arch files follow fundamental patterns
Generated arch files SHALL use fundamental library patterns for platform-specific code.

#### Scenario: Implement __main stub
- **WHEN** startup file is generated
- **THEN** it includes `__main()` function to satisfy GCC

#### Scenario: Platform-specific entry point
- **WHEN** startup file is generated
- **THEN** it includes platform-specific initialization code
