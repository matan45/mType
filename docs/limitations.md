# mType 1.0.0 Known Limitations

The 1.0.0 release is a stable-core release. The following advanced gaps are documented follow-up work and do not block the release.

## Language Server

- Range formatting is not implemented. The server advertises full-document formatting only.
- Some diagnostics and quick fixes are intentionally conservative when workspace indexing is still warming up.

## Language Semantics

- Nested classes and nested interfaces are rejected.
- Some generic cast cases remain conservative, especially where runtime generic type identity cannot be proven precisely.
- Generic `isClassOf`-style runtime type checks are intentionally limited in nested generic cases.

## Runtime and JIT

- JIT specialization is limited to proven hot paths and may fall back to VM execution for unsupported operations.
- Some multi-dimensional array and structure-of-arrays optimizations are intentionally conservative.
- Shape-specialized generics have eligibility limits, including user-defined equality/hash behavior and inline-slot budgets.

## Packaging

- GitHub Releases are the only 1.0.0 distribution channel.
- Marketplace publishing, MSI, Homebrew, apt, winget, Chocolatey, and Scoop packages are out of scope.
