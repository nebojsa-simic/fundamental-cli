## Context

The fun-c-compiler change introduces a 16-stage compilation pipeline where every stage produces a serializable intermediate result. Without caching, each `fun compile` invocation re-runs the full pipeline from source to assembly. The cache wraps around the existing pipeline, storing and retrieving per-file, per-stage results in `.fun/cache/`.

The compiler is built with the fundamental library (zero-stdlib). Cache implementation uses `fun_` prefixed APIs for file I/O, hashing, memory, and collections.

## Goals / Non-Goals

**Goals:**
- Fast incremental builds: skip stages whose inputs haven't changed
- Fast compiler development iteration: changing one stage only invalidates from that stage forward
- Correct invalidation: never serve stale cached results
- Zero configuration: caching is automatic, no flags needed

**Non-Goals:**
- Distributed or shared cache (e.g., across machines or CI)
- Cache size management or eviction policies (just `fun clean` to clear)
- Caching across different compiler versions (major version bump = full invalidation)

## Architecture

### Cache Directory Structure

```
.fun/cache/
├── manifest.bin              Cache manifest (all entries)
├── a3f2...bc.i               Cached stage output (content-addressed)
├── 7d1e...f4.tokens          Cached stage output
├── ...
└── e4a1...d7.s               Cached stage output
```

### Manifest Structure

```
manifest.bin
├── compiler_stages: {                    Per-stage compiler code hashes
│     "preprocessor-normalized": hash(normalize.c),
│     "preprocessor-includes":   hash(include.c),
│     "preprocessor-conditionals": hash(conditional.c),
│     "preprocessor-macros":     hash(expand.c),
│     "tokens":                  hash(tokenizer.c),
│     "lexed":                   hash(lexer.c),
│     "ast-parsed":              hash(parser.c + ast.c),
│     "ast-symbols":             hash(symcollect.c),
│     "ast-types":               hash(typeresolve.c),
│     "ast-resolved":            hash(nameresolve.c),
│     "ast-typed":               hash(typecheck.c),
│     "ir":                      hash(irgen.c),
│     "ir-abi":                  hash(abi_lower.c + abi_win64.c + abi_sysv.c),
│     "ir-registers":            hash(regalloc.c),
│     "assembly":                hash(asmemit.c),
│   }
├── shared_hash:                          Hash of foundational shared files
│     hash(type.c + symtab.c + token.c + serialize.c + linemap.c)
│
└── files: {                              Per-source-file entries
      "src/main.c": {
        mtime:       1710583200,
        source_hash: "fc29...a1",
        deps:        ["vendor/.../file.h", ...],
        deps_hash:   "8b3e...d2",
        stages: {
          "preprocessor-macros": { hash: "a3f2...bc", compiler_hash: "11..." },
          "tokens":              { hash: "7d1e...f4", compiler_hash: "22..." },
          ...
          "assembly":            { hash: "e4a1...d7", compiler_hash: "33..." },
        }
      }
    }
```

### Invalidation Flow

```
For file F, check cache validity:

1. mtime check (fast path)
   ├─ F.mtime unchanged AND all deps[].mtime unchanged
   │   → proceed to compiler check (step 3)
   └─ any mtime changed
       → content hash check (step 2)

2. Content hash check (correct path)
   ├─ hash(F + all deps) matches stored hash
   │   → update mtimes in manifest, proceed to compiler check (step 3)
   └─ hash differs
       → MISS from stage 1 (full recompile)

3. Per-stage compiler check
   For each stage S in pipeline order:
   ├─ shared_hash changed
   │   → MISS from this stage forward
   ├─ compiler_stages[S] changed vs stored compiler_hash
   │   → MISS from this stage forward
   └─ all match
       → HIT, use cached result

Resume compilation from earliest invalid stage.
```

### Cache Key Composition

Each cached stage output file is content-addressed:

```
filename = sha256(stage_input_hash + compiler_stage_hash)[0:16] + extension

Example: a3f2bc91d7e4f081.tokens
```

This means identical inputs always map to the same cache file, and different inputs never collide (within practical probability).

## Decisions

### Decision 1: Two-level invalidation (mtime then content hash)

**Rationale:** Mtime check is a single stat() call per file — near zero cost. It catches >99% of "nothing changed" cases. Content hashing only runs when mtime changes, which handles the "file was touched but not modified" case correctly.

**Alternatives considered:**
- Content hash only: Correct but requires reading every file on every build
- Mtime only: Fast but incorrect when files are touched without changing (e.g., git checkout)

### Decision 2: Per-stage compiler invalidation with flat shared hash (Option A)

**Rationale:** Each pipeline stage maps to specific compiler source files. When `parser.c` changes, only `--ast-parsed` and later stages need re-running. Foundational shared files (`type.c`, `symtab.c`, etc.) affect multiple stages, so a change to any of them invalidates all stages. This is slightly over-invalidating but simple and correct.

**Alternatives considered:**
- Per-stage dependency sets (Option B): More precise but requires maintaining a dependency mapping that can go stale, risking missed invalidation
- Whole-compiler invalidation: Too aggressive — changing `asmemit.c` would discard all cached preprocessing and parsing

### Decision 3: Content-addressed cache files

**Rationale:** Using content hashes as filenames means cache files are naturally deduplicated and can be verified. No need for a separate integrity check — if the file exists and the manifest points to it, it's valid.

**Alternatives considered:**
- Path-based naming (e.g., `src_main_c.tokens`): Simpler but doesn't handle content changes cleanly, requires overwriting

### Decision 4: Single manifest file

**Rationale:** One `manifest.bin` for the entire cache. Loaded once at pipeline start, written once at end. Avoids per-file manifest overhead and scattered metadata.

**Alternatives considered:**
- Per-file manifest (e.g., `src_main_c.manifest`): More granular but many small files, harder to keep consistent
- Database (SQLite): Over-engineering for this use case, adds dependency

## Risks / Trade-offs

**[Manifest corruption]** → Mitigation: If manifest fails to parse, delete it and rebuild from scratch. Cache files are content-addressed so they're still valid — just re-index.

**[Stale deps list]** → Mitigation: The deps list is populated during preprocessing (include resolution stage). If a new `#include` is added, the source file's mtime changes, triggering a full re-preprocess which discovers the new dependency.

**[Disk space growth]** → Mitigation: `fun clean` clears `.fun/cache/`. No automatic eviction — the cache is bounded by project size * number of stages. For fundamental-cli this is modest (tens of MB).

**[Clock skew / mtime unreliable]** → Mitigation: Mtime is only the fast path. Content hash is the correctness backstop. If mtime lies, we just do an extra hash — we never serve wrong results.
