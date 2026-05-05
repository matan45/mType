# Build Instructions for mType Language Server

The language server is built from the repository root. The root `premake5.lua`
is the canonical build configuration and also includes the `mtpm` package-manager
CLI.

## Prerequisites

- C++20 compiler
- Premake5
- On Linux, `build-essential` and `libcurl4-openssl-dev`
- On macOS, Xcode Command Line Tools

The language server uses the vendored `languageserver/vendor/nlohmann/json.hpp`
header. vcpkg or system `nlohmann-json` packages are not required.

## Generate and Build

### Windows

```batch
cd ..
premake5 vs2022
msbuild Interpreter.sln /p:Configuration=Release /p:Platform=x64 /m
```

### Linux/macOS

```bash
cd ..
premake5 gmake2
make config=release_x64
```

## Build Outputs

- mType CLI: `bin/mType/Release/x64/mType[.exe]`
- Package manager: `bin/mtpm/Release/x64/mtpm[.exe]`
- Language server: `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]`
- Language-server tests: `bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests[.exe]`

## Smoke Tests

```bash
bin/mType/Release/x64/mType --tests
bin/mtpm/Release/x64/mtpm --help
bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests
```

On Windows, use the same paths with backslashes and `.exe` suffixes.

## Legacy Standalone Files

`languageserver/premake5.lua` and `packagemanager/premake5.lua` are retained for
legacy local workflows, but the root Premake file is the source of truth used by
CI and should receive future build wiring changes.
