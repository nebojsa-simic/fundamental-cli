## ADDED Requirements

### Requirement: System scaffolds fun-cli skill
The system SHALL generate `.opencode/skills/fun-cli/SKILL.md` during `fun init`.

#### Scenario: Create skill directory
- **WHEN** project is scaffolded
- **THEN** `.opencode/skills/fun-cli/` directory is created

#### Scenario: Generate skill file
- **WHEN** project is scaffolded
- **THEN** `SKILL.md` is created from embedded template

#### Scenario: Scaffold alongside fundamental-expert
- **WHEN** `fun init` runs
- **THEN** both `fun-cli/SKILL.md` and `fundamental-expert/SKILL.md` are created
