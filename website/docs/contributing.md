---
title: Contributing
sidebar_position: 90
---

# Contributing

## Repository Layout

| Path | What's there |
|---|---|
| `mType/` | Compiler, parser, VM, JIT, project system, runtime, and tests. |
| `lib/` (separate repo) | Standard library — [matan45/mTypeLib](https://github.com/matan45/mTypeLib). Mirrored under `mType/tests/testFiles/lib/` for the test runner. |
| `languageserver/` | Standalone LSP server. |
| `mtype-vscode-extension/` | VS Code extension package. |
| `packagemanager/` | `mtpm` package manager. |
| `docs/` | Design notes and benchmark docs. |
| `website/` | This documentation site (Docusaurus). |
| `.github/workflows/` | CI, release, and docs deployment workflows. |

## Building from Source

### Windows

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
runPremake.bat
# open Interpreter.sln in Visual Studio 2022 and build Release x64
```

### Linux / macOS

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

### Build Outputs

- `bin/mType/Release/x64/mType[.exe]` — main interpreter
- `bin/mtype-launcher/Release/x64/mtype-launcher[.exe]` — `--build --exe` launcher
- `bin/mtpm/Release/x64/mtpm[.exe]` — package manager
- `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]` — LSP server
- `bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests[.exe]` — LSP tests

## Submodule Recovery

```bash
git submodule update --init --recursive
```

## Running the Test Suite

```bash
mType --tests
mType --tests --no-jit   # regression pass with JIT disabled
mType --test classes     # a single suite
```

Test fixtures live under `mType/tests/testFiles/` organized by feature.

| Category | Tests (approximate) |
|---|---|
| classes | 200+ |
| generics | 195+ |
| cast | 160+ |
| import | 130+ |
| controlFlow | 120+ |
| arrays | 105+ |
| interface | 100+ |
| await | 95 |
| lambda | 90 |

Other categories: annotation, reflection, stream, switch, value class, enhancedFor, network, workspace, packagemanager, exe, integration.

## Continuous Integration

[`/.github/workflows/ci.yml`](https://github.com/matan45/mType/blob/dev/.github/workflows/ci.yml) runs on every PR, push to `dev`, and tag.

| Platform | Toolchain | Build system |
|---|---|---|
| Windows (`windows-latest`) | MSVC v143 | Premake5 → `vs2022` → MSBuild |
| Linux (`ubuntu-24.04`) | GCC | Premake5 → `gmake2` → make |
| macOS (`macos-15-intel`) | Clang | Premake5 → `gmake2` → make |

CI runs the full interpreter test suite, the no-JIT regression pass, smoke tests for `mType` / `mtpm` / `mtype-language-server`, language-server tests, and VS Code extension packaging checks.

## Working on the Documentation Site

The site lives in `/website` (Docusaurus). To work on it locally:

```bash
cd website
npm install
npm run start
```

The dev server hot-reloads on edits. The `prestart` and `prebuild` hooks run `scripts/sync-assets.mjs`, which copies the canonical TextMate grammar from `mtype-vscode-extension/syntaxes/` and the existing `docs/*.md` reference files into the site tree. **Do not edit the synced files directly** — edit the source in `mtype-vscode-extension/` or `docs/` and rerun the sync.

## Pull Requests

- Branch from `dev`.
- Match the existing C++ style (modern C++17/20, RAII, smart pointers, no raw `new`/`delete`).
- Add or update tests under `mType/tests/testFiles/` for any user-visible behavior change.
- Run `mType --tests` and `mType --tests --no-jit` locally before opening the PR.
- Keep functions under ~50 lines and `.cpp` files under ~500 lines (see project conventions).

## License

MIT.
