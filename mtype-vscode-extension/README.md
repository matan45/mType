# mType Language Support for VS Code

This extension provides comprehensive language support for the mType programming language in Visual Studio Code.

## Features

- **Syntax Highlighting**: Complete syntax highlighting for mType files (.mt)
- **Code Completion**: Intelligent auto-completion for keywords, types, and built-in functions
- **Snippets**: Pre-built code templates for classes, methods, and namespaces
- **Language Configuration**: Proper bracket matching, auto-closing pairs, and comment handling
- **Code Execution**: Run mType files directly from VS Code
- **Test Runner**: Execute mType test suites with keyboard shortcuts

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

To enable code execution features, configure the path to your mType interpreter:

1. Open VS Code Settings (Ctrl+,)
2. Search for "mType Language Server"
3. Set the "Interpreter Path" to your mType executable location

Example: `C:\\path\\to\\mType\\bin\\mType\\Debug\\x64\\mType.exe`

## Usage

### Syntax Highlighting

Open any `.mt` file to automatically enable syntax highlighting for:
- Keywords (if, else, class, namespace, etc.)
- Types (int, float, string, Array, Set, etc.)
- Comments (// and /* */)
- Strings and numbers
- Operators and punctuation

### Code Completion

Trigger completion with Ctrl+Space or by typing:
- **Keywords**: All mType language keywords
- **Types**: Primitive and collection types
- **Snippets**: Templates for classes, methods, namespaces
- **Built-in Functions**: print, size, add, remove, etc.

### Code Execution

- **F5**: Run the current mType file
- **Ctrl+F5**: Run mType test suite
- **Right-click context menu**: "Run mType File" option

### Code Templates

Type these prefixes and press Tab to expand:
- `class` → Complete class template with constructor
- `method` → Method definition template
- `namespace` → Namespace definition template

## Language Features Supported

### Core Language
- Classes and objects
- Namespaces and imports
- Functions and methods
- Control flow (if/else, loops, switch)
- Variables and constants (final)
- Static members

### Advanced Features
- Generic collections (Array<T>, Set<T>, Map<K,V>, etc.)
- Annotations (@Script, @Autowired, @Update)
- Method chaining
- Object-oriented programming features

### Collection Types
- `Array<T>` - Dynamic arrays
- `Set<T>` - Unique element collections
- `Map<K,V>` - Key-value pairs
- `Stack<T>` - LIFO operations
- `Queue<T>` - FIFO operations

## Keyboard Shortcuts

| Command | Shortcut | Description |
|---------|----------|-------------|
| Run mType File | F5 | Execute current .mt file |
| Run mType Tests | Ctrl+F5 | Run all mType tests |
| Auto Complete | Ctrl+Space | Trigger code completion |

## File Association

This extension automatically activates for files with the `.mt` extension.

## Development

### Building the Extension

```bash
npm install          # Install dependencies
npm run compile      # Compile TypeScript
npm run watch        # Watch for changes
```

### Testing

1. Press F5 in VS Code to launch Extension Development Host
2. Open test mType files to verify features
3. Check language server functionality

### Debugging

- Use "Launch Extension" configuration to debug the extension
- Use "Attach to Language Server" to debug server features
- Check the Output panel for language server logs

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This extension is licensed under the MIT License.

## Changelog

### 1.0.0
- Initial release
- Syntax highlighting for mType language
- Code completion and snippets
- Language configuration
- Code execution commands
- Test runner integration