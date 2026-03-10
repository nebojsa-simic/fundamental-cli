## Context

Currently, developers must manually create project structure, build scripts, and configuration files when starting new fundamental library projects. This is error-prone, time-consuming, and leads to inconsistent project layouts. The fun CLI needs a scaffolding command to bootstrap projects with best practices baked in.

## Goals / Non-Goals

**Goals:**
- Generate complete, working fundamental project with single command
- Embed templates in binary for portability (no external template files)
- Copy fundamental from local `../fundamental` or git clone
- Scaffold minimal `arch/startup/` following fundamental principles
- Include LLM skill files for better AI assistance

**Non-Goals:**
- Interactive project configuration (wizard-style prompts)
- Multiple project templates (only one template for now)
- Custom project layouts or structures

## Decisions

### Decision 1: Embed templates as C strings
**Rationale:** Single binary distribution, no external file dependencies, works with curl installation.

**Alternatives considered:**
- External `templates/` directory: Easier to modify but requires file distribution
- Git submodule for templates: Adds complexity, requires git

### Decision 2: Copy fundamental from `../fundamental` first, git clone fallback
**Rationale:** Faster for local development, git clone ensures reproducibility for users.

**Alternatives considered:**
- Always git clone: Slower, requires network
- Only local copy: Doesn't work for first-time users

### Decision 3: Minimal arch scaffolding (startup only)
**Rationale:** Follows fundamental's own structure, provides necessary entry point without overwhelming users.

**Alternatives considered:**
- Complete arch with all modules: Too much boilerplate for simple projects
- No arch scaffolding: Users must create from scratch

### Decision 4: Scaffold two skills (fundamental-expert + fun-cli)
**Rationale:** Provides comprehensive LLM assistance for both fundamental library and fun CLI usage.

**Alternatives considered:**
- Only fundamental-expert: Missing CLI documentation
- Only fun-cli: Missing fundamental library knowledge

## Risks / Trade-offs

**[Template updates require binary rebuild]** → Mitigation: Version templates carefully, provide update mechanism in future

**[Large binary size from embedded templates]** → Mitigation: Templates are small (<10KB total), negligible impact

**[Git clone can fail due to network]** → Mitigation: Clear error messages, suggest manual copy

## Open Questions

- Should we support `--template` flag for future template variety?
- Should fun.ini version default to project version or track fun version?
