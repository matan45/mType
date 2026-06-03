# Changelog

This file summarizes user-visible changes for mType releases.

## v1.1.0

### Language and Runtime

- Stable-core release of the mType programming language: a statically typed, class-based language with interfaces, generics, pattern matching, nullable types, and value classes.
- Bytecode compilation and stack-based VM execution replace the older tree-walking runtime model.
- Optional x86-64 JIT support via AsmJit for hot-path execution, with `--no-jit` available for diagnostics and regression testing.
- Async / await support through `Promise<T>` and the VM event loop.
- Runtime reflection, annotations, and FFN native bindings for C++ integration.

### Build and Project Tooling

- `.mtproj` project files support include/exclude patterns, output paths, import search paths, aliases, and dependencies.
- `.mtworkspace` files support multi-project workspace builds.
- `.mtc` bytecode output is available with `mType --compile`.
- `.mtcLib` library packaging is available with `mType --build --lib`.
- Native executable builds are available with `mType --build --exe`, using the `mtype-launcher` binary to embed bytecode.
- GUI/windowed executable builds are supported with `mType --build --exe --gui`.

### Package Management

- Added `mtpm`, the mType package manager CLI.
- Package dependencies can be declared in `.mtproj` and installed into `mt_modules/`.
- Git-based package sources are supported through `github:owner/repo` shorthand or full git URLs.
- Package manifests use `mtpkg.json`, with support for package name, version, source directory, optional `.mtcLib` path, and dependency metadata.
- Local package publishing is available with `mtpm publish`; `MTYPE_REGISTRY` can override the local registry location.
- A central public package registry is not included in this release and remains planned future work.

### Developer Tooling

- Standalone C++ language server with completion, hover, diagnostics, references, semantic tokens, formatting, code actions, code lens, signature help, and path completion.
- VS Code extension source with LSP integration, debugger support, syntax highlighting, themes, formatting, run commands, and package-manager commands.
- Java mType editor repository linked from the README: https://github.com/matan45/mtype-code-editor.

### Standard Library and Tests

- Standard library coverage for core primitives, collections, streams, exceptions, math, networking, JSON, reflection, and testing.
- Built-in `mtest` framework for language-level tests.
- Large fixture suite covering language features, runtime behavior, package management, libraries, native bindings, and executable builds.

### Distribution and Known Limitations

- Prebuilt release archives are distributed through GitHub Releases.
- Marketplace publishing, MSI/Homebrew/apt/winget/Chocolatey/Scoop installers, and a central package registry are outside the v1.1.0 release scope.
- Advanced limitations are documented in `docs/limitations.md`.
