---
name: mtype-language-feature
description: Add, modify, or remove mType language features end to end across syntax, parser, AST, semantic/type behavior, bytecode compiler, VM/JIT runtime, tests, docs, language server, and VS Code extension. Use when the user asks for new syntax, semantic changes, compiler/runtime behavior changes, standard library language integration, diagnostics for language constructs, or feature parity across CLI, tests, LSP, and editor tooling.
---

# mType Language Feature

Use this skill for language-facing changes. Treat the language as a product surface: a feature is not done until parsing, semantics, runtime behavior, diagnostics, tests, and relevant tooling agree.

## Core Rule

Implement language changes as vertical slices. Start from an observable `.mt` fixture or editor behavior, then carry that one behavior through the smallest required stack. Do not update every layer speculatively; use the feature classification below to decide which surfaces are actually affected.

## 1. Classify the Feature

Before editing, classify the requested change:

- **Syntax**: lexer tokens, parser grammar, AST shape, formatter/syntax highlighting.
- **Semantic/type behavior**: declarations, resolution, modifiers, generics, overloads, diagnostics, type conversion.
- **Compile/runtime behavior**: bytecode emission, instruction semantics, VM execution, async, exceptions, native interop, JIT parity.
- **Standard library integration**: `lib/`, mirrored test library files, package/workspace implications.
- **Tooling**: language server completion, hover, references, semantic tokens, code actions, VS Code grammar/client behavior.
- **Documentation/examples**: README, website, limitations, language examples.

If the change touches syntax or public semantics, assume tests, docs, and editor/tooling need at least a quick relevance check.

## 2. Find the Existing Pattern

Search before designing. Prefer copying the closest existing feature path over inventing a new structure.

Load these references as needed:

- [language-feature-map.md](references/language-feature-map.md) for likely implementation surfaces.
- [test-fixture-patterns.md](references/test-fixture-patterns.md) for test fixture and suite conventions.
- [tooling-surface-checklist.md](references/tooling-surface-checklist.md) for LSP, VS Code, docs, and packaging checks.

Also read `CONTEXT.md` when the change involves parser, compiler, runtime, type catalog, bytecode optimization, or VM architecture vocabulary.

## 3. Build the First Vertical Slice

Start with one user-visible behavior:

1. Add or identify the narrowest failing test/fixture for the behavior.
2. Implement the minimum parser/semantic/compiler/runtime path needed for that fixture.
3. Run the narrowest relevant test command.
4. Repeat with error cases, edge cases, and tooling coverage only after the first slice works.

Keep pass and error behavior separate. Error tests should assert the intended diagnostic, not just any failure.

## 4. Preserve Parity

For runtime-visible language behavior:

- Treat `--no-jit` as the semantic reference unless the task proves an interpreter bug.
- Verify JIT-on and `--no-jit` behavior when bytecode, VM execution, values, call dispatch, arrays, strings, async, exceptions, or type metadata are affected.
- Update or add bytecode/compiler tests when behavior can be checked without a full script fixture.

For tooling-visible language behavior:

- Verify the language server can parse/analyze the new construct without drifting from compiler behavior.
- Update VS Code grammar/configuration only when text classification, indentation, brackets, comments, or snippets are affected.

## 5. Finish Criteria

Before reporting done:

- [ ] Positive `.mt` behavior is covered by a fixture, suite test, or equivalent public test.
- [ ] Invalid use has a diagnostic/error test when the feature introduces a new restriction.
- [ ] JIT and `--no-jit` parity is checked for runtime behavior.
- [ ] LSP/editor/docs impact is checked and updated when relevant.
- [ ] New syntax or public semantics are reflected in README/docs/examples when user-facing.
- [ ] Any temporary instrumentation or throwaway harness is removed.

Report the changed language behavior, the validation commands run, and any surface intentionally left unchanged.
