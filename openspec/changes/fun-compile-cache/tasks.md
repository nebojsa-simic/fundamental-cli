## 1. Cache Infrastructure

- [ ] 1.1 Define cache directory path constant (`.fun/cache/`)
- [ ] 1.2 Implement cache directory creation (create `.fun/cache/` if not exists)
- [ ] 1.3 Define manifest binary format: header (magic "FUNCCH", version u16, flags u32), compiler_stages section, shared_hash, file entries
- [ ] 1.4 Implement manifest serialization (write manifest.bin)
- [ ] 1.5 Implement manifest deserialization (read manifest.bin)
- [ ] 1.6 Implement manifest corruption detection: if parse fails, delete and return empty manifest
- [ ] 1.7 Implement content-addressed filename generation: sha256(input_hash + compiler_hash)[0:16] + extension

## 2. File Hashing

- [ ] 2.1 Implement file content hashing (sha256 of file contents using fundamental library)
- [ ] 2.2 Implement mtime retrieval (stat() wrapper for file modification time)
- [ ] 2.3 Implement dependency list hashing: sorted concat of all dep file hashes → single deps_hash
- [ ] 2.4 Implement compiler stage hashing: hash the source files that implement each pipeline stage
- [ ] 2.5 Implement shared code hashing: hash(type.c + symtab.c + token.c + serialize.c + linemap.c)

## 3. Invalidation Logic

- [ ] 3.1 Implement mtime check: compare source file mtime + all dep mtimes against manifest
- [ ] 3.2 Implement content hash check: hash source + deps, compare against manifest (runs only when mtime differs)
- [ ] 3.3 Implement mtime-only hit: when mtime matches, skip hash, return cache hit
- [ ] 3.4 Implement touch-but-not-modified case: when mtime differs but hash matches, update stored mtime, return hit
- [ ] 3.5 Implement per-stage compiler invalidation: compare current compiler_stages[S] against stored compiler_hash for each stage
- [ ] 3.6 Implement shared hash invalidation: if shared_hash changed, invalidate all stages
- [ ] 3.7 Implement earliest-invalid-stage detection: walk stages in pipeline order, find first miss
- [ ] 3.8 Write tests: mtime unchanged → hit (no hash computed)
- [ ] 3.9 Write tests: mtime changed, hash same → hit (mtime updated)
- [ ] 3.10 Write tests: mtime changed, hash different → miss
- [ ] 3.11 Write tests: compiler stage changed → partial miss from that stage
- [ ] 3.12 Write tests: shared hash changed → full miss

## 4. Cache Read/Write

- [ ] 4.1 Implement cache store: after each stage completes, write output to content-addressed file in `.fun/cache/`
- [ ] 4.2 Implement cache lookup: given stage name + input hash, check if content-addressed file exists
- [ ] 4.3 Implement manifest entry update: after successful compilation, update file entry with new hashes and stage references
- [ ] 4.4 Implement dependency list capture: during preprocessing (include resolution), record all included file paths into manifest entry
- [ ] 4.5 Write tests: store and retrieve a stage output, verify content matches
- [ ] 4.6 Write tests: content-addressed filenames are deterministic (same input → same filename)

## 5. Pipeline Integration

- [ ] 5.1 Integrate cache check at pipeline entry in compiler.c: load manifest, determine earliest invalid stage
- [ ] 5.2 Implement pipeline resume: load cached result for last valid stage, feed into earliest invalid stage
- [ ] 5.3 Implement manifest write at pipeline exit: update manifest with all new stage results
- [ ] 5.4 Implement compiler stage hash computation at startup: hash each stage's source files, store in manifest
- [ ] 5.5 Implement shared hash computation at startup: hash shared files, compare against manifest
- [ ] 5.6 Wire cache into `fun compile` command: automatic, no flags needed
- [ ] 5.7 Wire cache into `fun build --compiler=funcc`: per-file caching across multi-file compilation

## 6. Cache Clearing

- [ ] 6.1 Modify `fun clean` to delete `.fun/cache/` directory and all contents
- [ ] 6.2 Write test: `fun clean` removes cache directory
- [ ] 6.3 Write test: `fun clean` succeeds when no cache exists

## 7. End-to-End Tests

- [ ] 7.1 Write test: compile file, compile again unchanged → all stages cached (no re-run)
- [ ] 7.2 Write test: compile file, modify source, compile again → full re-run
- [ ] 7.3 Write test: compile file, modify header, compile again → full re-run
- [ ] 7.4 Write test: compile file, touch source (mtime changes, content same), compile again → cache hit
- [ ] 7.5 Write test: compile file, simulate compiler stage change, compile again → partial re-run from changed stage
- [ ] 7.6 Write test: compile file, simulate shared code change, compile again → full re-run
- [ ] 7.7 Write test: compile file, corrupt manifest, compile again → full re-run, manifest rebuilt
- [ ] 7.8 Write test: compile multiple files, change one, compile again → only changed file re-runs
