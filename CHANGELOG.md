# Changelog

## 1.0.0 - 2026-05-07

### Added

- Stable-core 1.0.0 release metadata for the interpreter, package manager, language server, documentation, and VS Code extension.
- `--version` support for `mType`, `mtpm`, and `mtype-language-server`.
- LSP `serverInfo.version` now reports the native release version.
- Root release files: `LICENSE`, `CHANGELOG.md`, `RELEASE.md`, and documented 1.0.0 limitations.
- GitHub Actions release packaging for versioned Windows x64, Linux x64, macOS x64 archives, and a VS Code `.vsix`.

### Changed

- VS Code extension activation no longer runs on `onStartupFinished`.
- The VS Code extension now resolves `mtype-language-server` from the configured path, bundled release binary locations, then `PATH`.
- Normal language-server startup logs to the output channel instead of showing success popups.
- CI now runs on pull requests, pushes to `dev`, and tags, with JIT/no-JIT test passes and extension packaging smoke coverage.

### Known Limitations

- Range formatting is intentionally disabled for 1.0.0; full-document formatting is supported.
- Nested classes and nested interfaces are rejected by design in this release.
- Some generic cast/runtime type-introspection cases remain intentionally conservative.
- Advanced JIT specialization gaps are documented and tracked as follow-up work.

See [docs/limitations.md](docs/limitations.md) for the release limitation list.
