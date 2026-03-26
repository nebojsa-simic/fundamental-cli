## ADDED Requirements

### Requirement: Overlay registry is fetched via HTTPS GET
The system SHALL fetch overlays from a registry using a simple HTTPS GET with no authentication required for reads.

#### Scenario: Successful overlay fetch
- **WHEN** `fun add <lib>@<version>` completes and a registry URL resolves
- **THEN** system performs GET to `https://overlays.fun-pkg.dev/<libname>/<version>/fun.vendor.toml` and writes the response to `vendor/.fun/<libname>.toml`

#### Scenario: Overlay not found in registry
- **WHEN** registry returns 404 for the requested library and version
- **THEN** system continues silently without writing an overlay file

#### Scenario: Registry unreachable
- **WHEN** HTTPS request fails due to network error or timeout
- **THEN** system emits a non-fatal warning and continues; build will fall through to Tier 2 or Tier 3

### Requirement: Overlay registry is separate from the package registry
The system SHALL use a distinct base URL for the overlay registry, with no coupling to the package registry endpoint.

#### Scenario: Registry URLs are independent
- **WHEN** the package registry base URL changes
- **THEN** the overlay registry URL is unaffected

### Requirement: Fetched overlays are not marked as generated
The system SHALL write registry-fetched overlays without `generated = true` so they are treated as hand-authored and never overwritten by auto-generation.

#### Scenario: Registry overlay survives regeneration
- **WHEN** a registry-fetched overlay exists and the library mtime changes
- **THEN** system uses the existing overlay as Tier 1 and does not overwrite it
