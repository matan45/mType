---
title: Architecture Overview
sidebar_position: 1
---

# Architecture Overview

mType compiles source to a stack-based bytecode VM with an optional x86-64 JIT. The repo is organized as a few well-defined layers:

```text
.mt source
   ↓
Lexer / Parser  →  AST  →  Optimizer  →  Bytecode Compiler  →  .mtc
                                                                ↓
                                                              VM  ──→ JIT (AsmJit)
```

## Top-level Components

| Component | Path | Responsibility |
|---|---|---|
| **Lexer** | `mType/lexer/` | Tokenization, source-location tracking, bracket balancing. |
| **Parser** | `mType/parser/` | Builds the AST. Sub-parsers per construct (class, interface, expression, statement, lambda, type). |
| **AST** | `mType/ast/` | Node types for every construct, plus the visitor scaffolding. |
| **Optimizer** | `mType/optimizer/` | Pluggable AST passes — constant folding, dead code elimination, unused decl elimination. |
| **Bytecode Compiler** | `mType/vm/compiler/` | AST → stack-based bytecode + class metadata. |
| **VM** | `mType/vm/runtime/` | Stack-based execution engine with specialized instruction executors. |
| **JIT** | `mType/vm/jit/` | AsmJit-backed x86-64 compilation of hot paths (scaffolding present). |
| **Type Registry** | `mType/types/` | Declared types, generic parameters, conversion rules. |
| **Runtime Types** | `mType/runtimeTypes/` | Class, function, and global definitions used at runtime. |
| **Services** | `mType/services/` | File reading, import management, top-level script driver. |

## Detailed Pages

- [Compilation Pipeline](pipeline.md)
- [Virtual Machine](vm.md)
- [JIT Compiler](jit.md)
