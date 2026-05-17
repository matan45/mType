# mType Language Support for VS Code

Comprehensive language support for the mType programming language in Visual Studio Code, providing professional IDE-level features including IntelliSense, debugging, code navigation, and more.

## Features Overview

### Language Support
- **Syntax Highlighting**: Complete TextMate grammar with semantic token highlighting
- **File Type Support**: Both `.mt` (source) and `.mtc` (compiled bytecode) files
- **Custom Themes**: mType Dark and Light color themes based on Atom One Dark
- **Custom File Icons**: Distinct icons for source files, bytecode files, and folders
- **Language Configuration**: Bracket matching, auto-closing pairs, smart indentation, and code folding

### Code Intelligence
- **IntelliSense**: Context-aware code completion for keywords, types, methods, and fields
- **Signature Help**: Real-time parameter hints while typing function/method calls
- **Go-to-Definition**: Jump to definitions across files (classes, methods, fields, variables)
- **Find All References**: Workspace-wide reference search with cross-file support
- **Code Lens**: Reference counts displayed above classes and methods
- **Semantic Highlighting**: Advanced token-based highlighting for better code readability

### Code Quality & Editing
- **Code Actions**: Quick fixes and refactoring suggestions
  - Auto-import suggestions for undefined symbols
  - Interface implementation code generation
  - Override annotation quick-fixes
- **Code Formatting**: Configurable document formatting
  - Indentation normalization (spaces/tabs)
  - Operator spacing
  - Import organization and sorting
  - Blank line insertion for readability
- **Import Management**: Auto-completion, validation, and diagnostics for import statements
- **Scope Analysis**: Variable visibility tracking and semantic analysis

### Debugging
- **Full Debug Support**: Set breakpoints, step through code, inspect variables
- **Exception Breakpoints**: Break on all or only uncaught exceptions
- **Debug Configuration**: Customizable launch configurations
- **Debug Adapter Protocol**: Full DAP implementation for mType

### Execution
- **Run Commands**: Execute mType files directly from VS Code
- **Project Builds**: Right-click any `.mtproj` to build, build as library, or build as executable
- **Context Menu Integration**: Right-click to run files
- **Terminal Integration**: Output shown in integrated terminal

### Package Manager
- **Command Palette Integration**: Install, add, remove, and list packages without leaving the editor
- **mType Modules Tree View**: Browse installed packages and their `.mt` files in the Explorer sidebar
- **Dedicated Output Channel**: Stream `mtpm` stdout/stderr to the `mType Package Manager` channel
- **Auto-Refresh**: Tree view updates after every install/add/remove and on external `mt_modules/` changes

## Installation

### From VSIX (Development)

1. Install the VS Code Extension Manager: `npm install -g vsce`
2. Clone this repository
3. Navigate to the extension directory: `cd mtype-vscode-extension`
4. Install dependencies: `npm install`
5. Compile the extension: `npm run compile`
6. Package the extension: `vsce package`
7. Install the generated .vsix file in VS Code

### Development Setup

1. Open the extension directory in VS Code
2. Run `npm install` to install dependencies
3. Press F5 to launch a new Extension Development Host window
4. Open an mType file (.mt) to test the extension

## Configuration

### mType Interpreter Path

To enable code execution and debugging features, configure the path to your mType interpreter:

1. Open VS Code Settings (Ctrl+,)
2. Search for "mType Language Server"
3. Set the "Interpreter Path" to your mType executable location

Example: `C:\\path\\to\\mType\\bin\\mType\\Debug\\x64\\mType.exe`

### Configuration Options

```json
{
  "mType.languageServer.path": "/path/to/mtype-language-server",
  "mTypeLanguageServer.interpreterPath": "/path/to/mType.exe",
  "mType.packageManager.path": "/path/to/mtpm"
}
```

### Configuration Reference

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `mType.languageServer.path` | string | "" | Path to the C++ language server executable (auto-detected if empty) |
| `mTypeLanguageServer.interpreterPath` | string | "" | Path to mType interpreter executable (used by the Run command) |
| `mType.packageManager.path` | string | "" | Path to `mtpm` executable (auto-detected if empty) |

Formatting options (tab size, spaces vs tabs) are sent by VS Code via standard LSP `FormattingOptions` — configure them in your VS Code editor settings.

## Usage

### Syntax Highlighting

Open any `.mt` or `.mtc` file to automatically enable syntax highlighting for:

- **Keywords**: Control flow, declarations, modifiers, async operations
- **Types**: Primitive types (int, float, string, bool) and collection types (Array, Set, Map, Stack, Queue)
- **Comments**: Single-line (`//`) and block (`/* */`) comments
- **Strings**: Double and single-quoted with escape sequences
- **Numbers**: Integers and decimals (including scientific notation)
- **Operators**: Arithmetic, comparison, logical, assignment, ternary
- **Annotations**: @Override, @Script, @Throw, and custom annotations
- **Functions & Classes**: With generic type parameter support

### IntelliSense & Code Completion

Trigger completion with **Ctrl+Space** or by typing. The extension provides:

#### Context-Aware Completions
- **Keywords**: Appropriate keywords based on current context (class body, method body, etc.)
- **Types**: Primitive types, collection types, user-defined classes, and interfaces
- **Instance Members**: Type-aware completions after object references (e.g., `object.`)
- **Static Members**: Class static methods and fields (e.g., `ClassName::`)
- **Constructor Completions**: After `new` keyword
- **Generic Type Parameters**: Type parameter suggestions in generic contexts
- **Import Completions**: Auto-complete imported symbols from other files
- **Scope Variables**: Parameters, local variables, and class fields

#### Collection Method Completions
Full API support for collection types:
- **Array<T>**: add, remove, get, set, size, contains, clear, forEach, etc.
- **Map<K,V>**: put, get, remove, containsKey, containsValue, size, keys, values
- **Set<T>**: add, remove, contains, size, clear
- **Stack<T>**: push, pop, peek, isEmpty, size
- **Queue<T>**: enqueue, dequeue, peek, isEmpty, size
- **List<T>**, **LinkedList<T>**, **HashMap<K,V>**, **HashSet<T>**

### Go-to-Definition

Navigate to definitions with **F12** or **Ctrl+Click**:

- Class definitions
- Method definitions (instance and static)
- Field definitions (instance and static)
- Variable declarations
- Constructor definitions
- Cross-file symbol lookup for imported types

### Find All References

Find all references to a symbol with **Shift+F12**:

- Class references (instantiations, type declarations, inheritance)
- Method calls (instance and static)
- Field access (instance and static)
- Variable usage
- Constructor calls
- Works across all files in workspace

### Code Lens

Reference counts appear above classes and methods:

```mtype
// 5 references
class MyClass {
    // 3 references
    public function myMethod() {
        // ...
    }
}
```

Click the reference count to view all references.

### Signature Help

Parameter hints while typing function calls:

```mtype
print(message: string, args: Array<object>) -> void
      ^^^^^^^ (current parameter)
```

Works with:
- User-defined functions and methods
- Constructors
- Built-in functions (print, hashCode, strLength, parsePrimitive)
- Generic methods

### Code Actions & Quick Fixes

Access quick fixes with **Ctrl+.** or click the lightbulb:

- **Auto-Import**: Automatically suggest imports for undefined symbols
- **Implement Interface**: Generate missing method stubs when implementing interfaces
- **Override Annotations**: Quick-fix suggestions for method overrides
- **Multiple Import Options**: Choose from multiple files when symbol exists in several locations

### Code Formatting

Format document with **Shift+Alt+F** or right-click → "Format Document"

The formatter automatically:
- Normalizes indentation (configurable spaces/tabs)
- Adds spaces around operators
- Organizes and sorts import statements
- Inserts blank lines between classes and methods
- Aligns brackets and parentheses
- Formats control structures consistently

### Import Management

Smart import handling:

- **Path Resolution**: Supports relative (`./`, `../`), absolute (`/`), and workspace-relative paths
- **Auto-Completion**: Import path suggestions while typing
- **Validation**: Real-time diagnostics for broken imports
- **Symbol Resolution**: Cross-file symbol lookup
- **Auto-Organization**: Sorts imports alphabetically and by depth

### Debugging

#### Setup Debug Configuration

Create a `.vscode/launch.json` file:

```json
{
  "version": "1.0.0",
  "configurations": [
    {
      "type": "mtype",
      "request": "launch",
      "name": "Debug mType File",
      "program": "${file}",
      "stopOnEntry": true,
      "interpreterPath": "${config:mTypeLanguageServer.interpreterPath}"
    }
  ]
}
```

#### Debug Features

- **Breakpoints**: Click in the gutter to set/remove breakpoints
- **Step Execution**: Step over, step into, step out
- **Variable Inspection**: View and inspect variables in debug sidebar
- **Call Stack**: Navigate the execution call stack
- **Exception Breakpoints**: Configure breaking on exceptions
  - All Exceptions: Break on every exception thrown
  - Uncaught Exceptions: Break only on uncaught exceptions (default)

#### Debug Configuration Options

| Option | Type | Description |
|--------|------|-------------|
| `program` | string | Absolute path to the mType file to debug (use `${file}` for current file) |
| `stopOnEntry` | boolean | Automatically pause when debugging starts |
| `trace` | boolean | Enable Debug Adapter Protocol logging |
| `interpreterPath` | string | Path to mType interpreter (use `${config:...}` to reference settings) |
| `args` | array | Command line arguments passed to the program |
| `cwd` | string | Working directory (use `${workspaceFolder}`) |

### Code Execution

Execute mType files from VS Code:

- **F5**: Run the current mType file (when `.mt` file is focused)
- **Right-click context menu**: "Run mType File" option
- **Command Palette**: Search for "Run mType File"

Output appears in the integrated terminal.

### Project Builds

Compile an `.mtproj` directly from VS Code — no need to drop to a terminal and remember the CLI flags. All four commands invoke `mType.exe` against the project path you right-click on, with the same `mTypeLanguageServer.interpreterPath` setting used by *Run mType File*.

| Command | What it does | CLI equivalent |
|---------|--------------|----------------|
| `mType: Build Project` | Build bytecode for the project | `mType.exe --build <path.mtproj>` |
| `mType: Build as Library (.mtcLib)` | Bundle the project as a reusable `.mtcLib` library | `mType.exe --build --lib <path.mtproj>` |
| `mType: Build as Executable` | Produce a standalone executable | `mType.exe --build --exe <path.mtproj>` |
| `mType: Build as Executable (GUI / Windowed)` | Same as *Build as Executable*, but uses the windowed-subsystem launcher so the produced exe doesn't pop a console window when double-clicked from Explorer on Windows. Linux/macOS: same as `--exe`. | `mType.exe --build --exe --gui <path.mtproj>` |

How to invoke:

- **Explorer right-click** on a `.mtproj` file — all four entries appear grouped together at the top.
- **Editor right-click** when the active editor is a `.mtproj` — same four entries.
- **Command Palette** (Ctrl+Shift+P) — entries are listed under `mType:` and only show when an `.mtproj` is selected/active.
- **Shift+F5** with a `.mtproj` selected runs the default *Build Project*.

All three commands stream output into a single reusable terminal named `mType Build` so consecutive builds reuse the same panel.

### Package Manager

Manage mType packages from the command palette (all commands live under the `mType` category):

| Command | What it does |
|---------|--------------|
| `mType: Install Packages` | Runs `mtpm install` in the workspace root |
| `mType: Add Package...` | Prompts for `name@version` plus an optional `--source github:user/repo`, then runs `mtpm add` |
| `mType: Remove Package...` | Shows installed packages in a quick-pick, then runs `mtpm remove` |
| `mType: List Packages` | Runs `mtpm list` and shows the output |

Installed packages appear in the **mType Modules** view in the Explorer sidebar. Each package shows its name and version; expand a package to browse its `.mt` source files and click any file to open it.

All `mtpm` output streams to the `mType Package Manager` output channel (`View → Output`, then pick the channel from the dropdown). The extension uses `child_process.spawn` with an argument array, so commands work identically under PowerShell, cmd, and bash without any quoting concerns.

The `mtpm` binary is located using this precedence: `mType.packageManager.path` setting → bundled paths (`bin/{platform}-{arch}/`, `bin/`, `server/`, extension root) → `PATH`. If it is not found, an error toast offers an **Open Settings** button that jumps directly to the setting.

## Themes

### mType Dark Theme

Based on Atom One Dark color scheme with optimized colors for mType syntax:

- **Comments**: Italic gray (#5c6370)
- **Keywords**: Magenta (#c678dd)
- **Native Keywords**: Orange (#ff8c00)
- **Primitive Types**: Yellow (#e5c07b)
- **Strings**: Green (#98c379)
- **String Escapes**: Cyan (#56b6c2)
- **Numbers**: Orange (#d19a66)
- **Function Names**: Blue (#7474ed)
- **Class Names**: Yellow (#e5c07b)
- **Operators**: Cyan (#56b6c2)
- **Members/Fields**: Red (#e06c75)

### mType Light Theme

Light color scheme for daytime coding (coming soon).

### Activating Themes

1. Open Command Palette (Ctrl+Shift+P)
2. Type "Color Theme"
3. Select "mType Dark" or "mType Light"

## Custom File Icons

The extension provides custom icons for:

- **`.mt` files**: mType source file icon
- **`.mtc` files**: mType compiled bytecode icon
- **mType folders**: Special folder icons for mType projects

### Activating File Icons

1. Open Command Palette (Ctrl+Shift+P)
2. Type "File Icon Theme"
3. Select "mType File Icons"

## Keyboard Shortcuts

| Command | Shortcut | Description |
|---------|----------|-------------|
| Run mType File | F5 | Execute current .mt file |
| Build Project | Shift+F5 | Build the selected `.mtproj` |
| Go to Definition | F12 | Jump to symbol definition |
| Find All References | Shift+F12 | Find all references to symbol |
| Trigger Suggest | Ctrl+Space | Open IntelliSense |
| Format Document | Shift+Alt+F | Format current file |
| Quick Fix | Ctrl+. | Show code actions/quick fixes |
| Duplicate Line | Ctrl+D | Duplicate current line down |
| Start Debugging | F5 | Start debugging session |

## Language Features Supported

### Core Language
- Classes and objects
- Interfaces with implementation
- Namespaces and imports
- Functions and methods (instance and static)
- Control flow (if/else, while, for, foreach, do-while, switch/case)
- Variables and constants (final modifier)
- Access modifiers (public, private, protected)
- Async/await operations
- Exception handling (try/catch/finally/throw)

### Object-Oriented Features
- Class inheritance (extends)
- Interface implementation (implements)
- Method overriding with @Override annotation
- Constructor definitions
- Static and instance members
- Abstract classes and methods
- Method visibility (public/private/protected)

### Type System
- Primitive types: int, float, string, bool, void
- Collection types with generics:
  - Array<T> - Dynamic arrays
  - Set<T> - Unique element collections
  - Map<K,V> - Key-value pairs
  - Stack<T> - LIFO operations
  - Queue<T> - FIFO operations
  - List<T> - List implementations
  - LinkedList<T> - Linked list
  - HashMap<K,V> - Hash-based maps
  - HashSet<T> - Hash-based sets
- User-defined classes and interfaces
- Generic type parameters
- Type inference
- Nullable types

### Advanced Features
- Annotations (@Script, @Override, @Throw, @Autowired, @Update, custom)
- Generic collections and classes
- Method chaining
- Lambda expressions (future support)
- Reflection (isClassOf)
- Native function integration

## File Association

This extension automatically activates for:

- **`.mt` files**: mType source files
- **`.mtc` files**: mType compiled bytecode files

## Development

### Building the Extension

```bash
npm install          # Install dependencies
npm run compile      # Compile TypeScript
npm run watch        # Watch for changes and auto-compile
```

### Project Structure

```
mtype-vscode-extension/
├── src/
│   ├── extension.ts              # Extension entry point (thin LSP client)
│   ├── languageClient.ts         # LSP client configuration
│   ├── debug/                    # Debug adapter
│   └── packages/                 # Package manager UI (mtpm integration)
├── syntaxes/
│   └── mtype.tmGrammar.json      # TextMate grammar
├── themes/
│   └── mtype-dark-theme.json     # Dark color theme
├── icons/
│   └── mtype-icon-theme.json     # File icon theme
├── language-configuration/
│   └── mtype-configuration.json  # Language configuration
└── package.json                  # Extension manifest
```

All language intelligence (completion, diagnostics, references, formatting, etc.) is provided by the C++ language server (`mtype-language-server`). The VS Code extension is a thin client that launches the server and forwards LSP messages.

### Testing

1. Press F5 in VS Code to launch Extension Development Host
2. Open test mType files in the `tests/` directory
3. Verify language server functionality:
   - IntelliSense and completions
   - Go-to-definition navigation
   - Find references
   - Code formatting
   - Debugging
4. Check Output panel for language server logs

### Debugging the Extension

- **Launch Extension**: Debug the extension host (F5)
- **Attach to Language Server**: Debug language server features
- **Debug Adapter**: Debug the debug adapter protocol implementation

Check the Output panel → "mType Language Server" for diagnostic logs.

## Architecture

The extension is a thin LSP client. All language intelligence is handled by the C++ language server (`mtype-language-server`), which speaks standard LSP over stdio. The server provides:

- **CompletionHandler**: Context-aware code completion
- **HoverHandler**: Type and documentation hover
- **DefinitionHandler**: Go-to-definition
- **ReferencesHandler**: Find all references (workspace-wide)
- **SignatureHelpHandler**: Parameter hints
- **SemanticTokensHandler**: Enhanced semantic highlighting (20 token types, 10 modifiers)
- **FormattingHandler**: Code formatting with import organization
- **CodeActionHandler**: Quick fixes and refactoring
- **CodeLensHandler**: Reference counts
- **DiagnosticsHandler**: Real-time error reporting
- **PathCompletionHandler**: Import path completion

The VS Code extension contributes static assets (TextMate grammar, themes, icons, language configuration) and common commands (Run, Format, Find References).

## Using mType Language Server with Other Editors

The C++ language server (`mtype-language-server`) speaks standard LSP over stdio and works with any LSP-capable editor.

### Neovim

```lua
vim.lsp.start({
    name = 'mtype',
    cmd = { '/path/to/mtype-language-server' },
    filetypes = { 'mtype' },
    root_dir = vim.fs.dirname(vim.fs.find({ '.mtproj', '.git' }, { upward = true })[1]),
})
```

Or with `nvim-lspconfig`, add a custom server config.

### Helix

In `~/.config/helix/languages.toml`:

```toml
[[language]]
name = "mtype"
scope = "source.mtype"
file-types = ["mt"]
language-servers = ["mtype-language-server"]

[language-server.mtype-language-server]
command = "/path/to/mtype-language-server"
```

### JetBrains (IntelliJ, CLion, etc.)

Install the [LSP Support](https://plugins.jetbrains.com/plugin/10209-lsp-support) plugin, then configure:

- **Executable**: `/path/to/mtype-language-server`
- **File pattern**: `*.mt`

### Emacs (eglot)

```elisp
(add-to-list 'eglot-server-programs
             '(mtype-mode . ("/path/to/mtype-language-server")))
```

### Zed

In `~/.config/zed/settings.json`:

```json
{
  "lsp": {
    "mtype-language-server": {
      "binary": { "path": "/path/to/mtype-language-server" }
    }
  }
}
```

### Sublime Text (LSP package)

In LSP settings:

```json
{
  "clients": {
    "mtype": {
      "enabled": true,
      "command": ["/path/to/mtype-language-server"],
      "selector": "source.mtype"
    }
  }
}
```

> **Note**: Only the VS Code extension provides icons, themes, and TextMate syntax highlighting. Other editors need their own syntax grammar (e.g., tree-sitter) for coloring.

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes following the project structure
4. Test thoroughly in Extension Development Host
5. Submit a pull request with detailed description

## Troubleshooting

### IntelliSense not working
- Verify the extension is activated (check status bar)
- Ensure file has `.mt` extension
- Check Output panel for errors

### Debugging not starting
- Configure `mTypeLanguageServer.interpreterPath` in settings
- Verify interpreter path is correct and executable exists
- Check launch.json configuration

### Formatter not working
- Enable `mTypeFormatter.formatOnSave` in settings
- Try manual format with Shift+Alt+F
- Check for syntax errors in the file

### Import suggestions missing
- Ensure imported files have `.mt` extension
- Verify import paths are correct
- Check that workspace contains the referenced files

### Package manager commands fail
- Set `mType.packageManager.path` to your `mtpm` executable, or place `mtpm` on `PATH`
- The error toast offers an **Open Settings** shortcut that jumps to the setting
- Inspect the `mType Package Manager` output channel for the spawn command, cwd, and exit code
- If a command appears to do nothing, an earlier `mtpm` invocation may still be running — only one operation runs at a time, and the second invocation surfaces a warning

## License

This extension is licensed under the MIT License.

## Changelog

### 1.0.0 - Initial Release

#### Language Support
- Complete syntax highlighting for mType language (.mt and .mtc files)
- TextMate grammar with 50+ token categories
- Language configuration (brackets, comments, indentation, folding)
- Custom Dark and Light color themes
- Custom file icon theme

#### Code Intelligence
- Context-aware IntelliSense with 10+ completion categories
- Signature help with parameter hints
- Go-to-definition (classes, methods, fields, variables, cross-file)
- Find all references (workspace-wide)
- Code lens showing reference counts
- Semantic token highlighting for enhanced readability

#### Code Quality
- Document formatting with 6 configurable options
- Import organization and auto-sorting
- Code actions and quick fixes (auto-import, implement interface, override)
- Real-time import validation and diagnostics
- Scope analysis and variable visibility tracking

#### Debugging
- Full Debug Adapter Protocol implementation
- Breakpoint support
- Exception breakpoints (all/uncaught)
- Variable inspection and call stack navigation
- Configurable launch configurations

#### Execution
- Run mType files from editor (F5)
- Context menu integration
- Terminal output integration

#### Package Manager
- Command-palette entries for `mtpm install`, `add`, `remove`, and `list`
- `mType Modules` tree view in the Explorer with per-package `.mt` file children
- Dedicated `mType Package Manager` output channel
- `mType.packageManager.path` setting with the same auto-discovery hierarchy as the language server
- Concurrency guard preventing overlapping `mtpm` invocations

#### Developer Experience
- Command palette integration
- Keyboard shortcuts
- Custom context menus
- Extension development and debugging support

---

**mType Language Support** - Professional IDE features for the mType programming language
