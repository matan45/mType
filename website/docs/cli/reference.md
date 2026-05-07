---
title: CLI Reference
sidebar_position: 1
---

# CLI Reference

Full flag list of the `mType` executable, grouped by purpose. Output is captured verbatim from `mType --help`.

## Run

| Flag | Description |
|---|---|
| `mType <script.mt>` | Run a script file. |
| `--debug <script.mt>` | Run with the debugger (breakpoints, stepping). |
| `--gc-stats <script.mt>` | Run and print GC statistics after execution. |
| `--jit-stats <script.mt>` | Run and print JIT statistics after execution. |
| `--no-jit <script.mt>` | Disable JIT compilation (JIT is on by default). |
| `--profile <script.mt>` | Run with the profiler (light mode: function timing). |
| `--profile=full <script.mt>` | Run with full profiler (timing + call graph + opcodes). |
| `--profile-output=json` | Emit profile report as JSON. |

## Compile

| Flag | Description |
|---|---|
| `--compile <script.mt>` | Compile to a `.mtc` bytecode file. |
| `--run-cached <file.mtc>` | Run pre-compiled bytecode. |

## Build

| Flag | Description |
|---|---|
| `--build [project.mtproj]` | Build a project (compile all files to bytecode). |
| `--build --lib [.mtproj]` | Build into a single `.mtcLib` library file. |
| `--build --exe [.mtproj]` | Build a standalone executable with embedded bytecode. |
| `--clean [project.mtproj]` | Remove compiled bytecode files. |

## Project

| Flag | Description |
|---|---|
| `--init <name> <include>` | Create a new `.mtproj` (e.g. `--init MyApp src/**/*.mt`). |
| `--init-workspace <name>` | Create a new `.mtworkspace`. |
| `--add <pattern> [.mtproj]` | Add an include pattern to the project. |
| `--remove <pattern> [.mtproj]` | Remove an include pattern. |

## Dependencies

| Flag | Description |
|---|---|
| `--deps [project.mtproj]` | Print the dependency tree. |
| `--deps --json [.mtproj]` | Export dependency graph as JSON. |
| `--deps --dot [.mtproj]` | Export dependency graph as Graphviz DOT. |
| `--deps --cycles [.mtproj]` | Detect circular dependencies. |
| `--deps --why <file> [.mtproj]` | Show the import chain to a file. |

## Tests

| Flag | Description |
|---|---|
| `--tests` | Run all test suites (JIT on). |
| `--tests --no-jit` | Run all test suites with JIT disabled (regression pass). |
| `--test <suite>` | Run a specific test suite (JIT on). |
| `--test <suite> --no-jit` | Run a specific suite with JIT disabled. |

## Benchmark

| Flag | Description |
|---|---|
| `--benchmark` | Run the interpreter benchmark suite. |
| `--benchmark=<script.mt>` | Run a single benchmark script. |
| `--benchmark-lexer=<path>` | Run a lexer-only microbenchmark on a `.mt` file. |
| `--benchmark-iterations=<N>` | Measured iterations per script (default 3). |
| `--benchmark-output=<fmt>` | Output format: `text` (default) or `json`. |

## Diagnostics

| Flag | Description |
|---|---|
| `--no-color` | Disable colored output (also `NO_COLOR` env var). |
| `--color=always\|auto\|never` | Force color mode (default: auto / TTY-detect). |
| `--find-script-classes <file>` | List `@Script` classes in a file. |
| `--test-script-objects <file>` | Demo: create objects and call methods from C++. |

## Info

| Flag | Description |
|---|---|
| `--help` | Print this help. |
| `--version`, `-v` | Print the version. |

## Examples

```bash
# One-shot
mType hello.mt

# Compile + run cached
mType --compile hello.mt
mType --run-cached hello.mtc

# Multi-file project
mType --init MyApp "src/**/*.mt"
mType --build MyApp.mtproj
mType --build --exe MyApp.mtproj

# Tests + benchmarks
mType --tests
mType --benchmark
```

## See Also

- [Projects](projects.md) — `.mtproj` and `.mtworkspace` configuration.
- [Package Manager](package-manager.md) — `mtpm` CLI.
- [Language Server](language-server.md) — wiring up your editor.
