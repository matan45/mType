---
title: Install
sidebar_position: 1
---

# Install

mType is distributed as source. Building takes a few minutes on a modern machine.

## Prerequisites

| Platform | Compiler | Build tool |
|---|---|---|
| **Windows** | MSVC v143 (Visual Studio 2022) | [Premake5](https://premake.github.io/) |
| **Linux** | GCC 11+ | Premake5 + GNU make |
| **macOS** | Clang (Xcode 14+) | Premake5 + GNU make |

You'll also need Git (with submodule support — mType pulls AsmJit as a submodule).

## Windows

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
runPremake.bat
# open Interpreter.sln in Visual Studio 2022 and build the Release x64 configuration
```

After the build finishes, the executables land under `bin/`:

- `bin/mType/Release/x64/mType.exe` — main interpreter
- `bin/mtpm/Release/x64/mtpm.exe` — package manager
- `bin/mtype-language-server/Release/x64/mtype-language-server.exe` — LSP server
- `bin/mtype-launcher/Release/x64/mtype-launcher.exe` — launcher used by `--build --exe`

## Linux / macOS

```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

Same `bin/` layout, with `[.exe]` suffixes dropped.

## Submodule Recovery

If you cloned without `--recursive`, initialize the AsmJit submodule before building:

```bash
git submodule update --init --recursive
```

## Verify the Install

```bash
mType --version
mType --help
```

You should see the version banner followed by the full flag list. If you see an error about missing AsmJit headers, your submodule didn't initialize.

## Next

→ [Quick Start](quick-start.md) — run your first script.
