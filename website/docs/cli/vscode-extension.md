---
title: VS Code Extension
sidebar_position: 4
---

# VS Code Extension

The extension under [`mtype-vscode-extension/`](https://github.com/matan45/mType/tree/dev/mtype-vscode-extension) provides:

- `.mt` and `.mtc` syntax highlighting (TextMate grammar) plus dark and light themes
- LSP client (stdio transport) launching the standalone language server
- DAP-based debugger integration
- Run, build, format, and package-manager commands
- File icons for `.mt`, `.mtc`, and mType project folders

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

The extension exposes three settings under `File → Preferences → Settings`, or in `settings.json`:

| Setting | Points to | Required? |
|---|---|---|
| `mTypeLanguageServer.interpreterPath` | The `mType` executable. Used by **Run mType File**, the three build commands, and as the default `interpreterPath` in `launch.json` debug configs. | Yes (the first invocation of *Run* / *Build* will show a "Set Path" prompt that opens this setting). |
| `mType.languageServer.path` | The `mtype-language-server` executable. Leave empty to auto-detect a copy bundled next to the extension. | No |
| `mType.packageManager.path` | The `mtpm` executable. The extension searches bundled `bin/{platform}-{arch}/`, `bin/`, `server/`, the extension root, and then `PATH`. | No |

Example `settings.json`:

```json
{
  "mTypeLanguageServer.interpreterPath": "C:\\path\\to\\bin\\mType\\Release\\x64\\mType.exe",
  "mType.languageServer.path": "C:\\path\\to\\bin\\mtype-language-server\\Release\\x64\\mtype-language-server.exe",
  "mType.packageManager.path": "C:\\path\\to\\bin\\mtpm\\Release\\x64\\mtpm.exe"
}
```

On Linux/macOS just drop the `.exe` suffix and use forward slashes.

## Commands

Every command appears under the `mType:` category in the Command Palette (`Ctrl+Shift+P`).

| Command ID | Palette title | Keybinding | Where it appears |
|---|---|---|---|
| `mtype.run` | Run mType File | `F5` | Editor right-click on `.mt`; Command Palette when an `.mt` file is active |
| `mtype.build` | Build Project | `Shift+F5` | Explorer / editor right-click on `.mtproj`; Command Palette when an `.mtproj` is selected |
| `mtype.buildLib` | Build as Library (.mtcLib) | — | Explorer / editor right-click on `.mtproj` |
| `mtype.buildExe` | Build as Executable | — | Explorer / editor right-click on `.mtproj` |
| `mtype.formatDocument` | Format Document | `Shift+Alt+F` | Editor right-click on `.mt` |
| `mtype.restartLanguageServer` | Restart Language Server | — | Command Palette |
| `mtype.packages.install` | Install Packages | — | Command Palette |
| `mtype.packages.add` | Add Package... | — | Command Palette (prompts for `name@version` and optional `--source`) |
| `mtype.packages.remove` | Remove Package... | — | Command Palette (quick-pick of installed packages) |
| `mtype.packages.list` | List Packages | — | Command Palette |

The package-manager commands shell out to `mtpm` and stream output to the **mType Package Manager** output channel — see [Package Manager (mtpm)](package-manager.md) for the underlying CLI semantics.

### How commands are surfaced

- **File-type gating.** `mtype.run` / `mtype.formatDocument` only appear when an `.mt` file is active; the three build commands only appear when an `.mtproj` is selected (in the Explorer) or active (in the editor). The Command Palette filter, right-click menus, and keybindings all respect the same `when` condition, so an irrelevant command will simply not show up.
- **Package-manager commands** are workspace-scoped — they run against the workspace root regardless of which file is selected, so they show up in the Command Palette unconditionally.
- **Terminals.** *Run mType File* uses a terminal named **mType** (fresh per invocation). The three build commands share a single reusable terminal named **mType Build**, so consecutive builds reuse the same panel.
- **Right-clicking an `.mtproj` in the Explorer** is the primary entry point for the build commands — the three entries appear grouped at the top of the context menu.

## Debugging

Debug an `.mt` file via VS Code's standard Run & Debug view. The extension contributes a `mtype` debug type that consumes `mTypeLanguageServer.interpreterPath` by default:

```json
{
  "type": "mtype",
  "request": "launch",
  "name": "Debug mType File",
  "program": "${file}",
  "stopOnEntry": true
}
```

Per-launch overrides for `interpreterPath`, `args`, and `cwd` are accepted — see the [extension README](https://github.com/matan45/mType/blob/dev/mtype-vscode-extension/README.md) for the full schema.

## Themes

- **mType Dark** — based on Atom One Dark.
- **mType Light** — coming soon.

The same TextMate grammar (`mtype.tmGrammar.json`, scope `source.mtype`) drives highlighting in this docs site, so the colors you see in VS Code match what's rendered in the [Language Reference](../language/classes.md) examples.

## Other Editors

The standalone language server is editor-agnostic. Wiring instructions for Neovim, Helix, JetBrains, Emacs, Zed, and Sublime are in the [extension README](https://github.com/matan45/mType/blob/dev/mtype-vscode-extension/README.md).

## See Also

- [Package Manager (mtpm)](package-manager.md) — what the `mType: Install / Add / Remove / List Packages` commands run under the hood.
- [Language Server](language-server.md) — capabilities of the LSP backend the extension launches.
- [Projects & Workspaces](projects.md) — `.mtproj` file format used by the build commands.
- [Extension README](https://github.com/matan45/mType/blob/dev/mtype-vscode-extension/README.md) — full reference, including debugger schema and other-editor wiring.
