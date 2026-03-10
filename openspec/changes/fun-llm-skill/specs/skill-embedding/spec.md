## ADDED Requirements

### Requirement: Skill template is embedded in binary
The fun-cli skill SHALL be stored as a C string template within the fun binary.

#### Scenario: Embed template
- **WHEN** fun is compiled
- **THEN** `fun-cli/SKILL.md` template is included as C string

#### Scenario: Access template
- **WHEN** `fun init` runs
- **THEN** embedded template is retrieved for scaffolding
