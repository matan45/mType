---
title: Compilation Pipeline
sidebar_position: 2
---

# Compilation Pipeline

```text
Source Code (.mt)
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Construction)
    ↓
AST Optimizer (optional, by optimization level)
    ├─ Constant Folding Pass
    ├─ Dead Code Elimination Pass
    └─ Unused Declaration Elimination Pass
    ↓
Bytecode Compiler
    ↓
Bytecode Program (.mtc)
    ↓
Virtual Machine (stack-based execution)
    ↓
Result
```

## Stages

### 1. Lexer

Translates source text to a token stream. Tracks source locations for diagnostics; balances brackets so the parser can recover from mismatched delimiters.

### 2. Parser

Constructs the AST from tokens. Specialized sub-parsers handle classes, interfaces, expressions, statements, lambdas, and types. Errors are accumulated rather than aborting on the first failure.

### 3. AST Optimizer

Optional, pluggable passes:

- **Constant Folding** — reduces `1 + 2` to `3` at compile time.
- **Dead Code Elimination** — removes unreachable branches. Preserves `@Script` and `@Throw` annotations.
- **Unused Declaration Elimination** — drops symbols nothing references.

Optimization level selects which passes run. Debug compiles skip optimization to keep source-line mapping faithful.

### 4. Bytecode Compiler

Lowers AST to a stack-based instruction stream. Emits:

- Instructions and their operands.
- A constant pool (strings, numbers).
- Class metadata (fields, method tables, generic parameters).
- Source locations for debugger and stack-trace use.

Output is either an in-memory `BytecodeProgram` (for `mType script.mt`) or a serialized `.mtc` file (for `mType --compile`).

### 5. Virtual Machine

Executes the bytecode. See [VM](vm.md).

## Execution Modes

1. **Direct** — `mType script.mt`. Compiles AST → bytecode in memory and executes.
2. **Compile** — `mType --compile script.mt`. Emits `script.mtc`.
3. **Cached** — `mType --run-cached script.mtc`. Loads bytecode and executes.
4. **Project Build** — `mType --build [project.mtproj]`. Compiles every source file to `.mtc`, optionally bundling into a `.mtcLib` or native `.exe`.

## See Also

- [VM](vm.md)
- [JIT](jit.md)
