# mType Language Server

A [Language Server Protocol (LSP)](https://microsoft.github.io/language-server-protocol/) implementation for the mType programming language.

## What is LSP?

The Language Server Protocol provides a standardized way for editors and IDEs to communicate with language-specific servers. This means you can write the language intelligence features **once** and have them work in **any** editor that supports LSP.

### Benefits

- ✅ **Universal Editor Support**: Works in VS Code, Vim, Neovim, Emacs, Sublime Text, IntelliJ, Eclipse, and more
- ✅ **Single Implementation**: Write once, run everywhere
- ✅ **Better Performance**: C++ implementation reuses mType's existing parser and type system
- ✅ **Industry Standard**: Follow the same protocol used by TypeScript, Rust, Python, and other modern languages

## Features

The mType Language Server provides:

- **IntelliSense/Auto-completion**: Context-aware completions for keywords, types, variables, functions
- **Hover Information**: Type information and documentation on hover
- **Diagnostics**: Real-time error checking and warnings
- **Future**: Go-to-definition, find references, formatting, etc.

## Building

### Prerequisites

- C++20 compatible compiler
- Premake5
- The vendored `languageserver/vendor/nlohmann/json.hpp` header

The canonical language-server build is part of the root `premake5.lua`. The
standalone `languageserver/premake5.lua` is kept only for legacy local workflows.

### Build with Premake5

**Windows:**
```bash
cd ..
premake5 vs2022
msbuild Interpreter.sln /p:Configuration=Release /p:Platform=x64
```

**Linux/macOS:**
```bash
cd ..
premake5 gmake2
make config=release_x64
```

Build outputs:

- Server: `bin/mtype-language-server/Release/x64/mtype-language-server[.exe]`
- Tests: `bin/mtype-language-server-tests/Release/x64/mtype-language-server-tests[.exe]`

## Installation

### System-wide Installation (Recommended)

**Linux/macOS:**
```bash
sudo cp bin/mtype-language-server/Release/x64/mtype-language-server /usr/local/bin/
```

**Windows:**
```bash
copy bin\mtype-language-server\Release\x64\mtype-language-server.exe "C:\Program Files\mType\"
# Add C:\Program Files\mType to PATH
```

### Local Installation

Place the executable in your project's `bin/` directory or anywhere in your PATH.

## Editor Configuration

### VS Code

The mType VS Code extension includes LSP support. Enable it in settings:

1. Open Settings (Ctrl+,)
2. Search for "mType Language Server"
3. Enable `mType › Language Server: Enable`
4. Set `mType › Language Server: Path` if needed (default: `mtype-language-server`)
5. Reload VS Code

**settings.json:**
```json
{
  "mType.languageServer.enable": true,
  "mType.languageServer.path": "mtype-language-server"
}
```

### Vim/Neovim with coc.nvim

**~/.config/nvim/coc-settings.json:**
```json
{
  "languageserver": {
    "mtype": {
      "command": "mtype-language-server",
      "args": ["--stdio"],
      "filetypes": ["mtype"],
      "rootPatterns": ["mtype.toml", ".git/"]
    }
  }
}
```

### Neovim with nvim-lspconfig

**~/.config/nvim/lua/lsp-config.lua:**
```lua
local lspconfig = require('lspconfig')
local configs = require('lspconfig.configs')

if not configs.mtype then
  configs.mtype = {
    default_config = {
      cmd = {'mtype-language-server', '--stdio'},
      filetypes = {'mtype'},
      root_dir = lspconfig.util.root_pattern('mtype.toml', '.git'),
      settings = {},
    },
  }
end

lspconfig.mtype.setup{}
```

Add to `~/.config/nvim/ftdetect/mtype.vim`:
```vim
autocmd BufRead,BufNewFile *.mt set filetype=mtype
```

### Emacs with lsp-mode

**~/.emacs.d/init.el or ~/.emacs:**
```elisp
(require 'lsp-mode)

;; Define mType mode (basic)
(define-derived-mode mtype-mode prog-mode "mType"
  "Major mode for mType programming language.")

(add-to-list 'auto-mode-alist '("\\.mt\\'" . mtype-mode))

;; Register mType language server
(add-to-list 'lsp-language-id-configuration '(mtype-mode . "mtype"))

(lsp-register-client
 (make-lsp-client
  :new-connection (lsp-stdio-connection '("mtype-language-server" "--stdio"))
  :major-modes '(mtype-mode)
  :server-id 'mtype))

(add-hook 'mtype-mode-hook #'lsp)
```

### Sublime Text with LSP

1. Install LSP package: `Package Control → Install Package → LSP`
2. Open `Preferences → Package Settings → LSP → Settings`
3. Add mType configuration:

```json
{
  "clients": {
    "mtype": {
      "enabled": true,
      "command": ["mtype-language-server", "--stdio"],
      "selector": "source.mtype"
    }
  }
}
```

4. Create syntax definition (save as `mType.sublime-syntax` in User folder):
```yaml
%YAML 1.2
---
name: mType
file_extensions: [mt]
scope: source.mtype
```

## Usage

The language server communicates via stdin/stdout using JSON-RPC protocol. It's designed to be started by your editor automatically.

**Manual testing:**
```bash
mtype-language-server --stdio
```

Then send LSP messages via stdin. For debugging, check your editor's LSP client logs.

## Supported LSP Features

| Feature | Status | Description |
|---------|--------|-------------|
| `textDocument/completion` | ✅ Implemented | Auto-completion |
| `textDocument/hover` | ✅ Implemented | Hover information |
| `textDocument/publishDiagnostics` | ✅ Implemented | Error checking |
| `textDocument/definition` | 🚧 Planned | Go-to-definition |
| `textDocument/references` | 🚧 Planned | Find references |
| `textDocument/formatting` | 🚧 Planned | Code formatting |
| `textDocument/semanticTokens` | 🚧 Planned | Semantic highlighting |
| `textDocument/codeAction` | 🚧 Planned | Quick fixes |
| `textDocument/prepareRename` | ✅ Implemented | Validate rename target (MYT-294) |
| `textDocument/rename` | ✅ Implemented | Symbol rename — boundary-safe token-driven edits; rejects keywords, builtins, literals, import paths, ambiguous symbols (MYT-294) |
| `textDocument/inlayHint` | ✅ Implemented | Parameter-name hints for user-defined calls (function/method/constructor) and inferred-type hints for untyped lambda parameters (MYT-295) |

## Architecture

The language server is built in C++ and reuses mType's existing components:

```
mType Language Server
├── DocumentManager - Tracks open files and their parsed state
├── Handlers
│   ├── CompletionHandler - Provides auto-completion
│   ├── HoverHandler - Provides hover information
│   └── DiagnosticsHandler - Provides error checking
└── Reuses mType Core
    ├── Lexer - Tokenization
    ├── Parser - AST construction
    ├── TypeChecker - Type validation
    └── Environment - Symbol resolution
```

## Development

### Project Structure

```
languageserver/
├── main.cpp                      # Entry point
├── src/
│   ├── MTypeLanguageServer.cpp   # Main server class
│   ├── DocumentManager.cpp       # Document tracking
│   ├── handlers/                 # LSP method handlers
│   │   ├── CompletionHandler.cpp
│   │   ├── HoverHandler.cpp
│   │   └── DiagnosticsHandler.cpp
│   └── utils/                    # Utilities
│       ├── JsonRpc.hpp           # JSON-RPC protocol
│       └── LSPTypes.hpp          # LSP type definitions
├── CMakeLists.txt
├── premake5.lua
└── README.md
```

### Adding New Features

1. **Add handler class** in `src/handlers/`
2. **Register handler** in `MTypeLanguageServer.cpp`
3. **Implement LSP method** following the protocol spec
4. **Reuse mType core** components (Parser, Environment, etc.)

Example - Adding go-to-definition:

```cpp
// src/handlers/DefinitionHandler.hpp
class DefinitionHandler {
public:
    std::optional<Location> handleDefinition(
        const std::string& uri,
        const Position& position
    );
private:
    DocumentManager* documentManager_;
};

// Register in MTypeLanguageServer.cpp
lspServer->onDefinition([this](const json& params) {
    return definitionHandler_->handleDefinition(
        params["textDocument"]["uri"],
        params["position"]
    );
});
```

## Debugging

Enable debug logging in your editor's LSP client. Most editors provide an output channel for LSP communication.

**VS Code:**
- View → Output → Select "mType Language Server"

**Neovim with coc.nvim:**
```vim
:CocCommand workspace.showOutput mtype
```

**Emacs:**
```elisp
(setq lsp-log-io t)
```

## Contributing

Contributions are welcome! Please:

1. Follow C++17 standards
2. Reuse existing mType components where possible
3. Add tests for new features
4. Update this README with new LSP capabilities

## License

MIT License - see LICENSE file

## Resources

- [LSP Specification](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/)
- [Language Server Guide](https://code.visualstudio.com/api/language-extensions/language-server-extension-guide)
- [mType Documentation](../README.md)
