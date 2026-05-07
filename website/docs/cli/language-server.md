---
title: Language Server
sidebar_position: 5
---

# Language Server

`mtype-language-server` is a standalone LSP server that reuses the core compiler's parser, project, and import infrastructure.

## Capabilities

- Completion (members, symbols, paths)
- Go-to-definition
- Hover
- Find references
- Code actions
- Code lens
- Signature help
- Formatting
- Diagnostics
- Semantic tokens

## Build

The language server is built as part of the main solution build:

```bash
runPremake.bat                      # Windows
# or: premake5 gmake2 && make       # Linux / macOS
```

Output: `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]`.

## Wiring Up an Editor

The server speaks LSP over stdio. Point your editor at the binary path; concrete examples for Neovim, Helix, Emacs, Zed, and Sublime are in the [extension README](https://github.com/matan45/mType/blob/dev/mtype-vscode-extension/README.md).

## See Also

- [VS Code Extension](vscode-extension.md)
- [`languageserver/README.md`](https://github.com/matan45/mType/blob/dev/languageserver/README.md) — build details and protocol notes.
