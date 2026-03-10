## ADDED Requirements

### Requirement: System copies fundamental-expert skill
The system SHALL copy `.opencode/skills/fundamental-expert/SKILL.md` from fundamental library to scaffolded project.

#### Scenario: Copy skill file
- **WHEN** project is scaffolded
- **THEN** `.opencode/skills/fundamental-expert/SKILL.md` is created

#### Scenario: Preserve skill content
- **WHEN** skill is copied
- **THEN** content matches source file exactly

### Requirement: Create opencode directory structure
The system SHALL create `.opencode/skills/` directory structure if it doesn't exist.

#### Scenario: Create skills directory
- **WHEN** project is scaffolded
- **THEN** `.opencode/skills/` directory structure is created

#### Scenario: Copy to correct location
- **WHEN** skill is copied
- **THEN** it's placed at `.opencode/skills/fundamental-expert/SKILL.md`
