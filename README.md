<div align="center">
  <img src="logo.png" alt="mType Logo" width="400"/>
</div>

# mType Programming Language

![Version](https://img.shields.io/badge/version-1.0.0-blue)
[![CI](https://github.com/matan45/mType/actions/workflows/ci.yml/badge.svg)](https://github.com/matan45/mType/actions/workflows/ci.yml)
[![Docs](https://img.shields.io/badge/docs-matan45.github.io%2FmType-4a7fc1)](https://matan45.github.io/mType/)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

📖 **Documentation:** https://matan45.github.io/mType/
📚 **Standard library:** https://github.com/matan45/mTypeLib

**mType Java editor:** https://github.com/matan45/mtype-code-editor

mType is a statically typed, class-based language with a bytecode VM, an x86-64 JIT, a project / workspace build system, a `mtpm` package manager, a standalone language server, and a VS Code extension. The repository ships the compiler, runtime, standard library, language server, package manager, extension source, and ~1,350+ test fixtures.

---

## Highlights

- Statically typed with classes, abstract / final / value classes, interfaces, generics, and pattern matching
- AOT compile to `.mtc` bytecode; in-process VM with optional AsmJit JIT
- `.mtcLib` libraries and standalone native executables via `--build --exe`
- `.mtproj` projects and `.mtworkspace` multi-project builds
- `mtpm` package manager CLI with git-based dependency sources
- Standalone C++ language server (completion, hover, diagnostics, references, semantic tokens, formatting, code actions)
- VS Code extension with LSP integration, debugger, themes, and syntax highlighting
- Async / await with `Promise<T>`, run on the VM event loop
- Reflection, annotations (built-in + user-defined with `@Retention` / `@Target`), and FFN native bindings
- Built-in `mtest` framework + ~1,350+ test fixtures under `mType/tests/testFiles`
- Cross-platform CI: Windows (MSVC v143), Linux (GCC), macOS (Clang)


---

## Standard Library

Distributed as `lib` (canonical packaging) with sources mirrored under `mType/tests/testFiles/lib/` for the test runner.

| Group | Modules |
|---|---|
| **Core** | `Object<T>`, `Function`, `BiFunction`, `Consumer`, `Predicate`, `Comparator`, `Iterable`, `Iterator` |
| **Primitives** | `Int`, `Float`, `Bool`, `String`, `Box<T>` |
| **Collections** | `ArrayList<T>`, `HashMap<K,V>`, `HashSet<T>`, `LinkedList<T>`, `Queue<T>`, `Stack<T>` |
| **Stream API** | `Stream<T>` with `filter`, `map`, `flatMap`, `distinct`, `sorted`, `limit`, `skip`, `reduce`, `forEach` |
| **Exceptions** | `Exception`, `RuntimeException`, `IllegalArgumentException`, `IndexOutOfBoundsException`, `NullPointerException`, `ClassNotFoundException` |
| **Math** | `Vec2f`, `Vec3f`, `Vec4f`, `Matrix3f`, `Matrix4f`, `Quaternion`, `Random` |
| **Network** | `Http`, `HttpResponse`, `TcpSocket`, `JsonApi` (`lib/net/`) |
| **Reflection** | `Class`, `Method`, `Field`, `Constructor`, `Annotation` (`lib/reflect/`) |
| **JSON** | `lib/json/` |
| **Testing — `mtest`** | `TestSuite`, `TestRunner`, `Assertions`, lifecycle annotations |

---

## Tooling

### Status

| Tool | Status | Notes |
|---|---|---|
| Compiler + Bytecode VM | ✓ Working | `.mt` → `.mtc`, JIT enabled by default |
| AsmJit JIT | ✓ Working | x86-64 hot-path JIT, opt-out with `--no-jit` |
| Project / workspace builds | ✓ Working | `.mtproj`, `.mtworkspace` parsed and built |
| `.mtcLib` libraries | ✓ Working | `--build --lib` produces a single packaged library |
| Native executables | ✓ Working | `--build --exe` embeds bytecode in launcher |
| `mtype-launcher` | ✓ Working | Stub binary used by `--build --exe`; bytecode is appended to a copy at build time. Shipped in release archives so end users can produce native exes without rebuilding mType. |
| Cross-platform CI | ✓ Working | Windows (MSVC v143), Linux (GCC), macOS (Clang, Intel) |
| Language server | ✓ Working | Standalone exe; full LSP capability set |
| VS Code extension | ⚠ Built, not on Marketplace | Source under `mtype-vscode-extension/`; package locally with `vsce` |
| Package manager (`mtpm`) | ⚠ CLI works, registry planned | `install` / `add` / `remove` / `list` / `init` work; sources are git-based, no central registry |
| Installer / distribution | ⚠ Partial | Prebuilt binaries published via [GitHub Releases](https://github.com/matan45/mType/releases); no `.msi`, Homebrew, or apt packaging yet |

Known advanced limitations are documented in [`docs/limitations.md`](docs/limitations.md).

### Compiler, VM, and JIT

Source files compile through Lexer → Parser → AST → Optimizer → Bytecode Compiler → VM. AsmJit-backed JIT promotes hot paths on x86-64. The VM provides async scheduling, arrays, boxing, reflection, and import resolution.

### Project System (`.mtproj`, `.mtworkspace`)

- `.mtproj` is the project file format (XML). It declares includes/excludes, output directory, import search paths, import aliases, and dependencies (with name, version range, and git source).
- `.mtworkspace` is the workspace file format. It lists member projects and a workspace-level output directory.
- Projects can build raw bytecode (`--build`), a `.mtcLib` library (`--build --lib`), or a native executable (`--build --exe`).

### `.mtc` Bytecode and `.mtcLib` Libraries

- `mType --compile script.mt` produces `script.mtc`.
- `mType --run-cached file.mtc` executes pre-compiled bytecode.
- `--build --lib` packages multiple `.mtc` units plus class metadata into a single `.mtcLib` for distribution.

### Native Executables

`mType --build --exe [project.mtproj]` builds a standalone executable by embedding the compiled bytecode into a launcher binary (`mtype-launcher`). The result runs without a separate `mType` install on the same OS/arch.

### Package Manager (`mtpm`)

```
mtpm install [project.mtproj]
mtpm add <pkg>@<ver> [--source github:user/repo]
mtpm remove <pkg>
mtpm list
mtpm init <name> <version>
mtpm --help
mtpm --version
```

`.mtproj` dependencies are resolved against git sources (e.g. `github:user/repo` or full git URLs). A central registry is on the roadmap.

### Language Server

The LSP server lives in [`languageserver/`](languageserver) and is built as a standalone executable (`mtype-language-server`). Capabilities:

- Completion, definition, hover, references
- Code actions, code lens, signature help
- Formatting, diagnostics, semantic tokens, path completion

It reuses the core parser, project, and import infrastructure. See [`languageserver/README.md`](languageserver/README.md) for editor wiring.

### VS Code Extension

The extension under [`mtype-vscode-extension/`](mtype-vscode-extension) provides:

- `.mt` syntax highlighting (TextMate grammar) plus dark and light themes
- LSP client (stdio transport) launching the standalone language server
- DAP-based debugger integration
- Commands: `mtype.run` (F5), `mtype.formatDocument` (Shift+Alt+F), `mtype.restartLanguageServer`

The extension is not yet published to the VS Code Marketplace; package locally with `npm run vscode:prepublish && vsce package`.

### FFN / Native Bindings

Native (FFN) bindings register C++ functions and classes with the VM's `NativeRegistry`. The standard library uses this for I/O, math, networking, and JSON, all surfaced as ordinary mType classes.


## Building from Source

### Windows

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
runPremake.bat
# open Interpreter.sln in Visual Studio and build
```

### Linux / macOS

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

The build produces:

- `bin/mType/Release/x64/mType[.exe]` — main interpreter
- `bin/mtype-launcher/Release/x64/mtype-launcher[.exe]` — `--build --exe` launcher
- `bin/mtpm/Release/x64/mtpm[.exe]` — package manager
- `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]` — LSP server
- `bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests[.exe]` — LSP tests

If you cloned without `--recursive`, initialize the AsmJit submodule:

```bash
git submodule update --init --recursive
```

---

## Repository Layout

- `mType/` — compiler, parser, VM, JIT, project system, runtime, and tests
- `languageserver/` — standalone LSP server
- `mtype-vscode-extension/` — VS Code extension package
- `packagemanager/` — `mtpm` source and build files
- `docs/` — design notes, benchmark docs, and implementation notes
- `.github/workflows/ci.yml` — cross-platform CI

---

## License

MIT License.
