# mType Language Feature Map

Use this map to find the existing path for a language feature. Search for a nearby implemented feature first, then follow the same shape.

## Syntax and AST

- Lexer/token surfaces: `mType/lexer/**`, `mType/token/**`.
- Parser surfaces: `mType/parser/**`; parser behavior is organized by grammar region.
- AST surfaces: `mType/ast/**`, especially `ast/nodes/{classes,expressions,functions,statements}/`.
- Type syntax: `parser::TypeParser` and `ast::GenericType`.

## Semantics and Type Behavior

- Type metadata and runtime catalog: `mType/environment/registry/**`, `mType/types/**`.
- Symbol/function/class registration: `mType/vm/compiler/registration/**` and registry classes under `environment/registry/`.
- Type checking, modifiers, overloads, generics, interfaces, annotations, null safety, and diagnostics usually have matching suites under `mType/tests/suites/`.

## Compiler and Runtime

- Bytecode program and instructions: `mType/vm/bytecode/**`.
- Bytecode emission: `mType/vm/compiler/**`, including visitor and registration subfolders.
- VM execution: `mType/vm/runtime/**`, especially executors, stack, context, and value handling.
- JIT behavior: `mType/vm/jit/**` and runtime JIT integration files.
- Optimizations: AST optimizer under `mType/optimizer/**`; bytecode optimization under `mType/vm/optimization/**`.

## Standard Library and Packages

- Canonical library: `lib/`.
- Test-runner mirror: `mType/tests/testFiles/lib/`.
- Project/workspace/package behavior: `mType/project/**`, `packagemanager/**`, workspace test fixtures.

## Tooling

- Standalone language server: `languageserver/**`.
- VS Code extension client/debug/package UI: `mtype-vscode-extension/src/**`.
- VS Code language configuration, grammar, snippets, themes: `mtype-vscode-extension/language-configuration/**`, `mtype-vscode-extension/syntaxes/**`, `mtype-vscode-extension/snippets/**`.

## Docs

- User overview: `README.md`.
- Current design/benchmark notes: `docs/**`.
- Website content: `website/**`.
- Project-specific agent context: `CONTEXT.md`, `CLAUDE.md`.
