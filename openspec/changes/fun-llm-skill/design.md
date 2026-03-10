## Context

LLMs need structured documentation to assist users with fun CLI effectively. Currently there's no official documentation for LLMs to reference. Embedding a skill file ensures every scaffolded project includes accurate, version-matched documentation.

## Goals / Non-Goals

**Goals:**
- Embed fun-cli skill template as C string in binary
- Scaffold .opencode/skills/fun-cli/SKILL.md during fun init
- Document all commands, flags, and workflows
- Include fun.ini format reference
- Provide common usage patterns and troubleshooting

**Non-Goals:**
- Dynamic documentation generation (static template)
- Separate skill repository (embedded in fun)
- Multiple skill formats (only opencode skill format)

## Decisions

### Decision 1: Embed as C string template (Option A)
**Rationale:** Single binary distribution, version-matched with fun, always available.

**Alternatives considered:**
- Separate file in fundamental repo: Version mismatch risk
- Generate dynamically: More complex, needs command introspection

### Decision 2: Scaffold alongside fundamental-expert skill
**Rationale:** Complementary documentation (CLI + library), both needed for effective development.

**Alternatives considered:**
- Only fun-cli skill: Missing fundamental library knowledge
- Only fundamental-expert: Missing CLI documentation

### Decision 3: Comprehensive command documentation
**Rationale:** LLMs need complete reference to provide accurate assistance.

**Alternatives considered:**
- Minimal documentation: Insufficient for complex queries
- Link to external docs: Requires network, may break

## Risks / Trade-offs

**[Skill updates require fun rebuild]** → Mitigation: Version carefully, document update process

**[Large embedded string]** → Mitigation: Skill is ~500 lines (~20KB), negligible for binary size

**[Documentation may become outdated]** → Mitigation: Update skill template when commands change

## Open Questions

- Should skill include troubleshooting for common errors?
- Should we support multiple LLM formats (not just opencode)?
