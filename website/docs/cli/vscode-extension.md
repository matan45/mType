---
title: VS Code Extension
sidebar_position: 4
---

# VS Code Extension

The extension under [`mtype-vscode-extension/`](https://github.com/matan45/mType/tree/dev/mtype-vscode-extension) provides:

- `.mt` and `.mtc` syntax highlighting (TextMate grammar) plus dark and light themes
- LSP client (stdio transport) launching the standalone language server
- DAP-based debugger integration
- File icons for `.mt`, `.mtc`, and mType project folders

## Commands

| Command | Default keybinding |
|---|---|
| `mtype.run` | `F5` |
| `mtype.formatDocument` | `Shift+Alt+F` |
| `mtype.restartLanguageServer` | — |

## Installing Locally

The extension is **not** yet published to the VS Code Marketplace. Package and install locally:

```bash
cd mtype-vscode-extension
npm install
npm run vscode:prepublish
npx vsce package
# install the resulting .vsix from VS Code: Extensions panel → ... → Install from VSIX
```

## Configuration

Set the path to your `mType` executable in your VS Code settings:

```json
{
  "mtype.interpreterPath": "C:\\path\\to\\bin\\mType\\Release\\x64\\mType.exe"
}
```

## Themes

- **mType Dark** — based on Atom One Dark.
- **mType Light** — coming soon.

The same TextMate grammar (`mtype.tmGrammar.json`, scope `source.mtype`) drives highlighting in this docs site, so the colors you see in VS Code match what's rendered in the [Language Reference](../language/classes.md) examples.

## Other Editors

The standalone language server is editor-agnostic. Wiring instructions for Neovim, Helix, JetBrains, Emacs, Zed, and Sublime are in the [extension README](https://github.com/matan45/mType/blob/dev/mtype-vscode-extension/README.md).

## See Also

- [Language Server](language-server.md)
