## ADDED Requirements

### Requirement: System scaffolds fundamental-expert skill from embedded template
The system SHALL generate `.opencode/skills/fundamental-expert/SKILL.md` from an embedded C string template during initialization.

#### Scenario: Create skill directory
- **WHEN** project is scaffolded
- **THEN** `.opencode/skills/fundamental-expert/` directory is created

#### Scenario: Generate skill file from template
- **WHEN** project is scaffolded
- **THEN** `SKILL.md` is created from embedded C string template

#### Scenario: Template is embedded in fun binary
- **WHEN** fun is compiled
- **THEN** fundamental-expert skill template is included as C string

### Requirement: Create opencode directory structure
The system SHALL create `.opencode/skills/` directory structure if it doesn't exist.

#### Scenario: Create skills directory
- **WHEN** project is scaffolded
- **THEN** `.opencode/skills/` directory structure is created

#### Scenario: Generate to correct location
- **WHEN** skill is generated
- **THEN** it's placed at `.opencode/skills/fundamental-expert/SKILL.md`
