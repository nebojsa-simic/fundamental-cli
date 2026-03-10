## Context

Currently, developers must manually create project structure, build scripts, and configuration files when starting new fundamental library projects. This is error-prone, time-consuming, and leads to inconsistent project layouts. The fun CLI needs a scaffolding command to bootstrap projects with best practices baked in.

**Workflow Decision:** `fun init` does not accept a project name. Users create the project directory first, cd into it, then run `fun init`. This avoids losing the path to the fun binary after initialization.

**Vendor Decision:** Only copy essential fundamental folders (`arch/`, `include/`, `src/`) to reduce vendor size and exclude development/test artifacts.

## Goals / Non-Goals

**Goals:**
- Generate complete, working fundamental project with single command
- Embed templates in binary for portability (no external template files)
- Copy only essential fundamental folders (`arch/`, `include/`, `src/`)
- Scaffold minimal `arch/startup/` following fundamental principles
- Include LLM skill files for better AI assistance
- Initialize current directory only (no project name parameter)

**Non-Goals:**
- Accepting project name as argument
- Copying entire fundamental repo (tests, openspec, node_modules, etc.)
- Interactive project configuration (wizard-style prompts)
- Multiple project templates (only one template for now)
- Custom project layouts or structures

## Decisions

### Decision 1: No project name parameter
**Rationale:** If `fun init something` creates a subdirectory and user runs `cd something`, they lose the path to the fun binary. The simplified workflow is: create project folder, cd into it, run `fun init`.

**Alternatives considered:**
- Accept project name: More convenient but breaks fun binary path
- Copy fun to new directory: Adds complexity, increases binary distribution complexity

### Decision 2: Copy only arch/, include/, src/ folders
**Rationale:** These are the only folders needed to build fundamental projects. Excluding tests/, openspec/, node_modules/ reduces vendor size by ~60-70%.

**Alternatives considered:**
- Copy entire repo: Includes unnecessary files, larger vendor size
- Copy individual files: More complex logic, harder to maintain
- Symlink to fundamental: Cross-platform issues, breaks self-contained principle

### Decision 3: Embed templates as C strings
**Rationale:** Single binary distribution, no external file dependencies, works with curl installation.

**Alternatives considered:**
- External `templates/` directory: Easier to modify but requires file distribution
- Git submodule for templates: Adds complexity, requires git

### Decision 4: Copy fundamental from `../fundamental` first, git clone fallback
**Rationale:** Faster for local development, git clone ensures reproducibility for users.

**Alternatives considered:**
- Always git clone: Slower, requires network
- Only local copy: Doesn't work for first-time users

### Decision 5: Minimal arch scaffolding (startup only)
**Rationale:** Follows fundamental's own structure, provides necessary entry point without overwhelming users.

**Alternatives considered:**
- Complete arch with all modules: Too much boilerplate for simple projects
- No arch scaffolding: Users must create from scratch

### Decision 6: Scaffold two skills (fundamental-expert + fun-cli)
**Rationale:** Provides comprehensive LLM assistance for both fundamental library and fun CLI usage.

**Alternatives considered:**
- Only fundamental-expert: Missing CLI documentation
- Only fun-cli: Missing fundamental library knowledge

## Risks / Trade-offs

**[User must create directory first]** → Mitigation: Document workflow clearly in help text and README

**[Excluding tests/ may confuse users who want to see examples]** → Mitigation: Document that tests are in fundamental repo, users can clone separately if needed

**[Template updates require binary rebuild]** → Mitigation: Version templates carefully, provide update mechanism in future

**[Git clone can fail due to network]** → Mitigation: Clear error messages, suggest manual copy

## Open Questions

- Should we support `--template` flag for future template variety?
- Should fun.ini version default to project version or track fun version?
- Should we provide an option to copy entire repo for users who want tests/examples?
