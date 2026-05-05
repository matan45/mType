<div align="center">
  <img src="logo.png" alt="mType Logo" width="400"/>
</div>

# mType Programming Language

![Version](https://img.shields.io/badge/version-0.2.0-blue)
[![CI](https://github.com/matan45/mType/actions/workflows/ci.yml/badge.svg)](https://github.com/matan45/mType/actions/workflows/ci.yml)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

mType is a statically typed programming language with a bytecode VM, JIT support, a package manager, an LSP server, and a separate VS Code extension. The repository also includes the standard library, test fixtures, and project/workspace tooling.

## Highlights

- Bytecode execution with `.mtc` output and VM runtime support
- Native executable builds through the project system
- `.mtcLib` library packaging and dependency linking
- Cross-platform CI on Windows, Linux, and macOS
- Language server for editor integration
- Separate VS Code extension package under `mtype-vscode-extension/`
- Package manager CLI in `packagemanager/`
- Test corpus organized under `mType/tests/testFiles`

## Language Features

### Core Syntax
- Classes, abstract classes, inheritance, and polymorphism
- Interfaces and multiple interface implementation
- Constructors, static methods, method overloading, and modifiers
- Final classes, final methods, and final variables
- Access modifiers and annotations
- Generics with constraints and inference
- Casting and import aliases

### Control Flow
- `for`, `while`, and `do while` loops
- Enhanced `for each` iteration
- `switch` with `case` and `default`
- `try`, `catch`, and `finally`
- Recursive functions and methods
- Pattern matching support where applicable

### Expressions and Data
- Arrays
- Lambda expressions and higher-order functions
- Async / await with `Promise`
- String interpolation
- String pooling
- Reflection
- Boxed primitive wrappers for `String`, `Int`, `Float`, and `Bool`
- Collections and Stream API support
- Network library support
- Error handling and runtime exceptions

### Library Surface
- Standard collections such as `List`, `Map`, `Set`, `Queue`, and `Stack`
- Stream operations including map, filter, reduce, and chaining
- Reflection types such as `Class`, `Method`, `Field`, `Constructor`, and `Annotation`
- Shared library code in `lib/`

## Tooling

### Compiler, VM, and JIT
- Source files compile to bytecode and execute through the virtual machine
- Hot paths can be JIT compiled with asmjit on supported platforms
- The runtime includes async scheduling, arrays, boxing, reflection, and import resolution

### Project System
- `.mtproj` is the project file format
- `.mtworkspace` supports workspace-level builds and cross-project imports
- Projects can build bytecode, libraries, and executables
- Library output is packaged as `.mtcLib`

### Package Manager
- `mtpm` is the package manager CLI in `packagemanager/`
- Supported commands include `install`, `add`, `remove`, `list`, and `init`
- Package resolution uses `.mtproj` dependencies and `mtproj.lock`
- The package manager is tested against registry and project fixtures under `mType/tests/testFiles/packagemanager`

### Language Server
- The LSP server lives in `languageserver/`
- It provides completion, hover, diagnostics, definition, references, formatting, semantic tokens, code actions, and signature help
- It reuses the core parser and project/import infrastructure
- Editor setup details live in [`languageserver/README.md`](languageserver/README.md)

### VS Code Extension
- The VS Code extension is packaged separately under `mtype-vscode-extension/`
- It provides syntax highlighting, completion, debugging, formatting, themes, and language client integration
- Extension packaging and TypeScript sources live in the extension directory

## CI and Tests

The GitHub Actions pipeline in [`.github/workflows/ci.yml`](.github/workflows/ci.yml) runs on:

- Windows with MSVC
- Linux with GCC
- macOS with Clang

The CI job builds the root solution and runs:

- interpreter tests
- `mtpm --help`
- language server tests

Feature coverage is organized under `mType/tests/testFiles`, with groups for arrays, classes, interfaces, generics, lambdas, async, reflection, streams, workspace builds, package manager flows, and executable tests.

## Building

### Windows

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
runPremake.bat
# open Interpreter.sln and build
```

### Linux / macOS

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

The build also produces:

- `bin/mtpm/Release/x64/mtpm[.exe]`
- `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]`
- `bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests[.exe]`

If you cloned without `--recursive`, initialize the asmjit submodule with:

```bash
git submodule update --init --recursive
```

## Running

```bash
mType hello.mt
mType --compile hello.mt
mType hello.mtc
```

## Repository Layout

- `mType/` - compiler, parser, VM, project system, runtime, and tests
- `lib/` - standard library sources
- `languageserver/` - standalone LSP server
- `mtype-vscode-extension/` - VS Code extension package
- `packagemanager/` - `mtpm` source and build files
- `docs/` - design notes, benchmark docs, and implementation notes

## Current Status

Implemented and covered by the repository:

- core language parsing and execution
- classes, interfaces, generics, lambdas, async / await, and reflection
- arrays, imports, annotations, modifiers, and control flow
- bytecode, VM, JIT, and runtime support
- project and workspace builds
- package manager CLI
- language server
- VS Code extension
- benchmark and test infrastructure

## License

MIT License.
