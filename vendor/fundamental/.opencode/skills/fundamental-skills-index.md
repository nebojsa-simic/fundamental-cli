---
name: fundamental-skills-index
description: Index of all Fundamental Library AI agent skills
license: MIT
compatibility: Central index for all fundamental-* skills
metadata:
  author: fundamental-library
  version: "1.0"
  category: index
---

# Fundamental Library - AI Agent Skills Index

This is the central index for all Fundamental Library skills for AI coding agents (Opencode, Claude Code).

---

## Available Skills

### Core Operations

| Skill | File | Description |
|-------|------|-------------|
| **File I/O** | [fundamental-file-io.md](fundamental-file-io.md) | Read, write, append files, check existence |
| **Memory** | [fundamental-memory.md](fundamental-memory.md) | Allocate, free, copy, fill, compare memory |
| **Console** | [fundamental-console.md](fundamental-console.md) | Output text, progress bars, error messages |
| **Directory** | [fundamental-directory.md](fundamental-directory.md) | Create, list, remove directories, iterate files |

### Data & Strings

| Skill | File | Description |
|-------|------|-------------|
| **String** | [fundamental-string.md](fundamental-string.md) | Copy, join, template, convert strings |
| **Collections** | [fundamental-collections.md](fundamental-collections.md) | Arrays, hashmaps, sets, red-black trees |

### Advanced Operations

| Skill | File | Description |
|-------|------|-------------|
| **Async** | [fundamental-async.md](fundamental-async.md) | Await results, poll status, timeout |
| **Process** | [fundamental-process.md](fundamental-process.md) | Spawn processes, capture stdout/stderr, terminate |
| **Config** | [fundamental-config.md](fundamental-config.md) | Load configuration, cascade sources, get values |
| **Network** | [fundamental-network.md](fundamental-network.md) | Simple async TCP/UDP client, address parse, overflow-buffered receive |
| **Platform** | [fundamental-platform.md](fundamental-platform.md) | Detect OS and architecture, convert to string |

---

## How to Use Skills

### For AI Agents

When implementing Fundamental Library code, reference the relevant skill:

1. **Identify the task**: "I need to read a file"
2. **Find the skill**: fundamental-file-io.md
3. **Copy the pattern**: Use the example as a template
4. **Adapt to context**: Modify paths, sizes, error handling as needed

### Example Workflow

```
User: "Read a config file and parse it"

Agent workflow:
1. Load fundamental-file-io.md for file reading pattern
2. Load fundamental-memory.md for buffer allocation
3. Load fundamental-string.md for string parsing
4. Combine patterns into working code
```

---

## Skill Format

Each skill follows this structure:

```markdown
---
name: fundamental-<name>
description: <what it does>
license: MIT
metadata:
  author: fundamental-library
  version: "1.0"
  related: <other skills>
---

# Fundamental Library - <Name> Skill

## Quick Reference
| Task | Function | Example |

## Task: <Task Name>
```c
// Copy-paste example code
```

## See Also
- Links to related skills
- Links to header files
```

---

## Design Principles

All skills follow these principles:

1. **Example-First**: Working code before explanations
2. **Complete Patterns**: Allocate → Operation → Error Check → Use → Cleanup
3. **Error Handling**: Every example shows error checking
4. **Memory Safety**: Every allocation has a corresponding free
5. **Cross-References**: Skills link to related skills

---

## Cross-Reference Map

```
fundamental-file-io.md
    └──→ fundamental-memory.md (for buffers)
    └──→ fundamental-directory.md (for paths)

fundamental-console.md
    └──→ fundamental-string.md (for formatting)
    └──→ fundamental-memory.md (for progress buffers)

fundamental-collections.md
    └──→ fundamental-memory.md (for allocation/destruction)

fundamental-config.md
    └──→ fundamental-string.md (for templating)
    └──→ fundamental-console.md (for error output)

fundamental-async.md
    └──→ fundamental-file-io.md (for async file ops)
    └──→ fundamental-process.md (process spawn depends on async)

fundamental-process.md
    └──→ fundamental-async.md (AsyncResult returned by fun_process_spawn)

fundamental-platform.md
    └──→ fundamental-console.md (for logging platform info)
    └──→ fundamental-async.md (for spawning platform-specific processes)

fundamental-network.md
    └──→ fundamental-async.md (for non-blocking I/O patterns)
    └──→ fundamental-memory.md (for receive buffer allocation)
```

---

## Quick Start by Task

**"I want to..."**

| Task | Start With |
|------|-----------|
| Read a file | fundamental-file-io.md |
| Write output | fundamental-console.md |
| Store data | fundamental-collections.md |
| Parse text | fundamental-string.md |
| Load settings | fundamental-config.md |
| Run a command | fundamental-process.md |
| Allocate memory | fundamental-memory.md |
| List files | fundamental-directory.md |
| Detect OS/arch | fundamental-platform.md |
| Connect TCP/UDP | fundamental-network.md |

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024 | Initial release with 8 core skills |

---

## See Also

- **[fundamental-expert](../fundamental-expert/SKILL.md)** - Comprehensive library guide
- **[include/](../../include/)** - Header files for complete API reference
- **[tests/](../../tests/)** - Working test examples
