---
name: refactor
description: Code refactoring discipline for the mType C++ engine and the mtype-vscode-extension TypeScript codebase. Use when the user asks to refactor, simplify, split oversized files (.cpp/.hpp/.ts), extract helpers, inline/rename across files, restructure folder layout, apply Visitor/Strategy/Factory or other SOLID-driven patterns, consolidate duplication, or reduce parameter lists — while preserving behavior.
---

# Refactor

Refactor in small, behavior-preserving steps. Read surrounding code, identify current seams and tests, then make the smallest change that improves locality without changing public behavior.

Scope of this skill — all C++/TypeScript code in the repo:
- `mType/` — core engine (lexer, parser, AST, VM, JIT, runtime).
- `languageserver/src/` — LSP server (handlers, document manager, workspace symbol index).
- `packagemanager/src/` — mtpm (dependency resolver, lockfile, git source).
- `mtype-vscode-extension/src/` — VS Code client extension (TypeScript).

For language-design work on `.mt` source itself, use a different skill.

## Hard limits (review gates)

- Methods/functions: **≤ 50 lines**.
- `.cpp`, `.hpp`, and `.ts` files: **≤ 500 lines**, except for documented deep-module exceptions in `AGENTS.md` (top-level parsers, `TypeParser.cpp`, `StatementTypeDetector.cpp`). Test infrastructure under `mType/tests/` and `languageserver/tests/` is exempt by design — registration-heavy code, not behavior. Never invent new exceptions silently — if a file must exceed 500 lines, justify it and add it to `AGENTS.md`.
- Parameters: **≤ 5**. Bundle related inputs into a params struct/interface.
- Classes live in feature/domain folders, never a flat shared dump.

Enumerate current violators before planning:
```
powershell -ExecutionPolicy Bypass -File .Codex/skills/refactor/scripts/check-file-sizes.ps1
```
Exit 0 = clean, exit 1 = hard violations exist (documented exceptions are noted but don't fail).

## Plan-first checklist

Do not start editing until every box below is checked.

1. **Scope locked** — name the target files and the single refactor type (split / extract / inline / rename / restructure / pattern). Mixing types in one pass is the #1 source of regressions.
2. **Behavior contract written** — list what must stay observable: public methods, exception types thrown, bytecode opcodes emitted, on-disk layout. This is what tests must still prove after the refactor.
3. **Affected call sites listed** — `Grep` for every symbol you intend to move/rename. Note headers, `.cpp` includes, registration sites (`BytecodeCompiler/registration/`), test files, the VS Code extension's parser references.
4. **Tests located** — find the suite covering the touched code (`mType/tests/suites/`, extension tests). If none, write one *before* moving code so the refactor has a safety net.
5. **Build status baseline** — the user runs the build (see [feedback_user_runs_build](../../memory/feedback_user_runs_build.md)). Confirm baseline is green before refactoring.
6. **Single PR-sized step** — if the change would touch >10 files or cross module boundaries, split into sequential refactors and reconfirm after each.

## Operation playbook

Pick exactly one — see [REFERENCE.md](REFERENCE.md) for full mechanics, mType-specific anchors, and worked examples:

- **Split oversized file** — extract cohesive subset to a sibling file/folder, update includes/imports, keep the public type unchanged.
- **Extract / inline / rename** — mechanical refactors with full call-site sweep. Renaming a registered class name (`runtimeTypes/klass/`) also touches the bytecode constant pool — handle deliberately.
- **Restructure module layout** — move files between context folders. Update `premake5.lua` filters / `tsconfig.json` paths, and verify the build picks up the new locations.
- **Pattern-driven (SOLID)** — apply Visitor / Strategy / Factory / Observer. Justify against `AGENTS.md`'s SOLID guidance and the deletion test (if removing the abstraction concentrates complexity in one place, keep it; if it just relocates it across N callers, drop it).

## After every refactor

- Run the narrowest useful test suite, then broaden if shared behavior changed.
- Remove stale `MYT-*` ticket comments and commented-out code in touched regions; preserve genuine "why" comments.
- Delete dead includes, declarations, and helpers proved unused — confirm via `Grep`, not just intuition. Public-API / plugin-facing / reflection-facing code stays unless evidence is overwhelming.
- Report any limit still violated and **why** it was deferred to a separate refactor. Don't leave silent violations.

## Anti-patterns

- Mixing rename + restructure + split in one commit — bisect-hostile.
- Extracting helpers "for testability" that have no second caller and never will — that's pass-through code; the deletion test fails. See [improve-codebase-architecture](../improve-codebase-architecture/SKILL.md).
- Adding `// removed X` or `_unused` placeholder comments instead of deleting cleanly.
- Refactoring a file you haven't read end-to-end. The 500-line limit makes this cheap; honor it.
