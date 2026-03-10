## Why

LLMs need structured documentation about fun CLI commands to assist users effectively. Currently, LLMs lack knowledge of available commands, flags, and workflows. Embedding a skill file in the binary ensures every scaffolded project includes accurate, version-matched documentation.

## What Changes

- **New**: Embedded `fun-cli/SKILL.md` template as C string in fun binary
- **New**: Automatic scaffolding of `.opencode/skills/fun-cli/SKILL.md` during `fun init`
- **New**: Comprehensive command documentation with examples and workflows
- **New**: fun.ini format reference with examples
- **New**: Common usage patterns and troubleshooting guidance
- **Modified**: `fun init` now scaffolds two skills: `fundamental-expert` and `fun-cli`

## Capabilities

### New Capabilities

- `skill-embedding`: Template storage as C string within fun binary
- `skill-scaffolding`: Automatic generation of fun-cli skill during project initialization
- `command-documentation`: Structured documentation of all fun commands, flags, and examples
- `workflow-examples`: Common usage patterns and best practices for LLM guidance

### Modified Capabilities

- `project-scaffolding`: Extended to include fun-cli skill alongside fundamental-expert skill

## Impact

- **Code**: `templates/skills/fun-cli/SKILL.md.tmpl` as embedded C string
- **fun init**: Modified to scaffold two skill files instead of one
- **Documentation**: Single source of truth for fun CLI usage (embedded template)
- **LLM Integration**: Opencode and other LLMs can reference skill for accurate guidance
