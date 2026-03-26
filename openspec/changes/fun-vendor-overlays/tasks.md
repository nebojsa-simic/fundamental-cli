## 1. Overlay Format — Parser

- [ ] 1.1 Define `VendorOverlay` struct: `name`, `generated`, `sources[]`, `includes[]`, `sources_extra[]` (platform-specific), `dependencies[]`
- [ ] 1.2 Define `VendorOverlayResult` and `vendor-overlay.h` header
- [ ] 1.3 Implement TOML array parser: `key = ["a", "b", "c"]` → string array (extend existing ini parser)
- [ ] 1.4 Implement `vendor_overlay_load(path)`: parse `vendor/.fun/<libname>.toml` into `VendorOverlay`
- [ ] 1.5 Implement `vendor_overlay_write(path, overlay)`: serialise `VendorOverlay` back to TOML
- [ ] 1.6 Write test: load overlay with all sections, verify field values
- [ ] 1.7 Write test: load overlay missing optional sections, verify defaults
- [ ] 1.8 Write test: round-trip write then load, verify identical fields

## 2. Overlay Resolution — Tier 1 (Read Existing Overlay)

- [ ] 2.1 Implement `vendor_overlay_dir_ensure()`: create `vendor/.fun/` if absent
- [ ] 2.2 Implement `vendor_resolve_from_overlay(libname, platform)`: load overlay, expand sources + platform sources_extra, prefix with library root path
- [ ] 2.3 Implement `vendor_overlay_is_stale(overlay_path, lib_dir_path)`: compare overlay mtime to library directory mtime
- [ ] 2.4 Implement circular dependency detection in sub-dependency resolution
- [ ] 2.5 Write test: resolve sources for fundamental given a pre-authored overlay
- [ ] 2.6 Write test: stale detection — mtime of lib dir newer than overlay

## 3. Overlay Auto-Generation — Tier 2 (Include Scan)

- [ ] 3.1 Implement `include_scan_file(path)`: tokenize a `.c` or `.h` file, extract all `#include "fundamental/..."` paths, return module name set
- [ ] 3.2 Implement `include_scan_sources(src_paths[], count)`: call `include_scan_file` on all source files, union module sets
- [ ] 3.3 Implement `include_scan_vendor_headers(module_set, vendor_include_root)`: for each module in set, scan `vendor/fundamental/include/fundamental/<module>/*.h` for new `#include "fundamental/..."` directives; add discovered modules; repeat until fixed point
- [ ] 3.4 Implement `module_expand_sources(module_name, vendor_root, platform)`: collect all `.c` files under `vendor/fundamental/src/<module>/` and `vendor/fundamental/arch/<module>/<platform>/`
- [ ] 3.5 Implement `vendor_autogen_fundamental(src_paths[], count, vendor_root, platform)`: run 3.2 → 3.3 → 3.4, build `VendorOverlay`, write to `vendor/.fun/fundamental.toml`
- [ ] 3.6 Write test: scan a minimal `src/` with `#include "fundamental/async/async.h"`, verify async + memory modules found
- [ ] 3.7 Write test: transitive discovery — config module pulls in hashmap via header scan
- [ ] 3.8 Write test: fixed-point termination — no infinite loop on circular headers

## 4. Source Scanning — Arch Directory

- [ ] 4.1 Extend `build_scan_sources()` to also scan `arch/<platform>/` for `.c` files alongside `src/`
- [ ] 4.2 Write test: project with `arch/build/windows-amd64/*.c` — files appear in scan result

## 5. Build Generator — Dynamic Vendor Resolution

- [ ] 5.1 Implement `vendor_resolve_all(src_files[], count, vendor_root, platform)`: enumerate `vendor/` subdirectories (excluding `.fun/`), resolve each via tier 1 → tier 2 → warn, return combined file list
- [ ] 5.2 Inject startup files unconditionally at head of vendor file list
- [ ] 5.3 Replace hardcoded vendor string in `arch/build/windows-amd64/generator.c` with call to `vendor_resolve_all()`
- [ ] 5.4 Replace hardcoded vendor string in `arch/build/linux-amd64/generator.c` with call to `vendor_resolve_all()`
- [ ] 5.5 Emit warning to stderr when a vendor library cannot be resolved (no overlay, not fundamental-style, no compiler)
- [ ] 5.6 Write test: `fun build` on project with fundamental only — generated script matches expected file list
- [ ] 5.7 Write test: `fun build` after adding a hand-authored overlay for a fake library — overlay sources appear in script
- [ ] 5.8 Write test: `fun build` with unknown library and no overlay — warning emitted, library omitted, build continues

## 6. Overlay Registry — Fetch on `fun add`

- [ ] 6.1 Implement `overlay_registry_fetch(libname, version, out_path)`: HTTP GET `https://overlays.fun-pkg.dev/<libname>/<version>/fun.vendor.toml`, write to `out_path` on 200; silent on 404; warn on network error
- [ ] 6.2 Integrate fetch into `cmd_add_execute()`: after vendoring completes, call `overlay_registry_fetch`; write to `vendor/.fun/<libname>.toml` without `generated = true`
- [ ] 6.3 Write test: simulate 200 response — overlay file written correctly
- [ ] 6.4 Write test: simulate 404 — no file written, no error returned
- [ ] 6.5 Write test: simulate network failure — warning emitted, no file written, `fun add` returns success

## 7. Smoke Test

- [ ] 7.1 Run smoke test (`smoke-test-windows-amd64.bat`) end-to-end: `fun init` → vendor → `fun build` → run binary — verify pass
- [ ] 7.2 Manually verify `vendor/.fun/fundamental.toml` is written on first build of a fresh project
- [ ] 7.3 Manually verify second build skips scanning (no re-generation of overlay)
