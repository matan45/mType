# mType Tooling Surface Checklist

Use this only after classifying a language change. Not every feature needs every item.

## Language Server

Check `languageserver/**` when the change affects:

- Parsing or diagnostics shown in editors.
- Completion items, hover text, go-to-definition, references, symbols, semantic tokens, formatting, or code actions.
- Project/workspace resolution that differs from CLI behavior.

Validation target: run `mtype-language-server-tests` when language-server behavior changes.

## VS Code Extension

Check `mtype-vscode-extension/**` when the change affects:

- Syntax highlighting or TextMate tokenization.
- Brackets, comments, indentation, auto-closing pairs, or surrounding pairs.
- Snippets, commands, package manager UI, debugger behavior, or language client startup.

Validation target: run extension build/package commands only when extension source or package metadata changes.

## Documentation and Examples

Update docs when the feature changes public language behavior:

- `README.md` for language tour, CLI, package, LSP, or status-table changes.
- `website/**` when the public docs site has matching content.
- `docs/limitations.md` if a limitation is removed, added, or narrowed.
- `examples/**` when example projects should demonstrate the feature.

## Packaging and Release Surface

Check package/release workflows when the change affects:

- Files required at runtime in release packages.
- Standard library layout.
- `mtpm` package semantics.
- VSIX contents or language-server executable expectations.

Relevant files usually include `.github/workflows/**`, `premake5.lua`, `packagemanager/**`, and extension package metadata.
