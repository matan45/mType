# mType Programming Language

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

A modern, statically-typed programming language with object-oriented features, namespaces, and a clean syntax inspired by TypeScript and Java.

## 🌟 Features

### Language Features
- **Static Typing**: Strong type system with `int`, `float`, `string`, `bool`, and `void` types
- **Object-Oriented Programming**: Classes, inheritance, constructors, and access modifiers
- **Namespaces**: Organize code with namespace support and scope resolution
- **Modern Control Flow**: if/else, switch/case, for/while/do-while loops
- **Functions**: First-class functions with support for static and final methods
- **Enums**: Enumerated types for better type safety
- **Modules**: Module system for code organization

### Syntax Highlights
```mtype
// Variable declarations with type annotations
int count = 42;
float pi = 3.14159;
string message = "Hello, mType!";
bool isReady = true;

// Functions
function factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Classes with inheritance
class Animal {
    protected string name;
    
    public constructor(string n) {
        this.name = n;
    }
    
    public function speak() {
        return "Some sound";
    }
}

class Dog extends Animal {
    public constructor(string n) {
        super(n);
    }
    
    public function speak() {
        return "Woof! I'm " + this.name;
    }
}

// Namespaces
namespace Math {
    function abs(float x) {
        if (x < 0) {
            return -x;
        }
        return x;
    }
}

using Math::abs;
```

## 🚀 Getting Started

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+ (optional, for build management)

### Building from Source

#### Using Make
```bash
git clone https://github.com/yourusername/mType.git
cd mType
make
```

#### Using CMake
```bash
git clone https://github.com/yourusername/mType.git
cd mType
mkdir build && cd build
cmake ..
make
```

#### Using Visual Studio
1. Open the solution file `mType.sln` in Visual Studio
2. Build → Build Solution (Ctrl+Shift+B)

### Running Your First Program

Create a file `hello.mt`:
```mtype
function main() {
    string greeting = "Hello, World!";
    print(greeting);
    return 0;
}
```

Run it:
```bash
./mtype hello.mt
```

## 📁 Project Structure

```
mType/
├── mType/
│   ├── ast/              # Abstract Syntax Tree nodes
│   │   ├── AstNode.hpp    # Base AST node class
│   │   ├── Expressions.hpp # Expression nodes
│   │   ├── Statements.hpp  # Statement nodes
│   │   └── Declarations.hpp # Declaration nodes
│   ├── core/             # Core language components
│   │   ├── Token.hpp      # Token definitions
│   │   ├── Value.hpp      # Value type system
│   │   ├── Result.hpp     # Result/Error handling
│   │   └── Environment.hpp # Variable environment
│   ├── frontend/         # Compiler frontend
│   │   ├── Lexer.hpp      # Lexical analyzer
│   │   └── Parser.hpp     # Parser
│   ├── interpreter/      # Runtime execution
│   │   ├── Interpreter.hpp # Main interpreter
│   │   └── Evaluator.hpp   # Expression evaluator
│   ├── runtime/          # Runtime components
│   │   ├── Function.hpp    # Function objects
│   │   ├── Klass.hpp       # Class runtime
│   │   ├── Module.hpp      # Module system
│   │   └── Namespace.hpp   # Namespace support
│   ├── extensions/       # Built-in libraries
│   │   ├── Builtin.hpp     # Built-in functions
│   │   └── Api.hpp         # Standard API
│   ├── utils/           # Utilities
│   │   └── FileCollector.hpp # File operations
│   └── boot/            # Entry point
│       └── Main.cpp        # Main entry
└── tests/               # Test suite
    ├── LexerTest.cpp      # Lexer unit tests
    └── IntegrationTest.cpp # Integration tests
```

## 🧪 Testing

### Running Tests

#### Unit Tests
```bash
# Build and run lexer tests
g++ -std=c++17 tests/LexerTest.cpp mType/frontend/Lexer.cpp mType/core/Token.cpp -o lexer_test
./lexer_test

# Run with verbose output
./lexer_test -v
```

#### Integration Tests
```bash
# Build integration tests
g++ -std=c++17 tests/IntegrationTest.cpp mType/frontend/Lexer.cpp mType/core/Token.cpp -o integration_test

# Run standard test suite
./integration_test

# Interactive mode for testing code snippets
./integration_test -i

# Test a specific file
./integration_test -f examples/sample.mt
```

### Test Coverage
- ✅ Lexical Analysis (97.8% pass rate)
  - Single and multi-character tokens
  - Keywords and identifiers
  - String literals with escape sequences
  - Number literals (integers and floats)
  - Comments (single-line and multi-line)
  - Error handling
- 🚧 Parser (In Development)
- 🚧 Semantic Analysis (Planned)
- 🚧 Interpreter (In Development)

## 🔧 Language Specification

### Data Types
| Type | Description | Example |
|------|-------------|---------|
| `int` | 32-bit integer | `42`, `-100` |
| `float` | 64-bit floating point | `3.14`, `-0.001` |
| `string` | Text string | `"Hello"`, `"Line\nBreak"` |
| `bool` | Boolean | `true`, `false` |
| `void` | No value | Used for functions |

### Operators
| Category | Operators |
|----------|-----------|
| Arithmetic | `+`, `-`, `*`, `/`, `%` |
| Comparison | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| Logical | `&&`, `!`, `||` |
| Assignment | `=`, `+=`, `-=` |
| Increment | `++`, `--` |

### Keywords
```
if, else, switch, case, default, while, for, do, break, continue,
function, return, class, new, this, extends, super, constructor,
enum, int, float, bool, string, void, public, private, final,
static, namespace, using, module, null, true, false
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

### Development Guidelines
- Follow C++17 standards
- Use the Result<T> pattern for error handling
- Write tests for new features
- Update documentation as needed

## 📊 Current Status

- [x] Lexical Analysis
- [x] Token Generation
- [x] Error Handling Framework
- [ ] Parser Implementation (In Progress)
- [ ] AST Generation
- [ ] Semantic Analysis
- [ ] Type Checking
- [ ] Interpreter
- [ ] Standard Library
- [ ] Module System
- [ ] Optimization

## 📝 Examples

### Fibonacci Sequence
```mtype
function fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

function main() {
    for (int i = 0; i < 10; i++) {
        print(fibonacci(i));
    }
    return 0;
}
```

### Class Example
```mtype
class Rectangle {
    private float width;
    private float height;
    
    public constructor(float w, float h) {
        this.width = w;
        this.height = h;
    }
    
    public function area() {
        return this.width * this.height;
    }
    
    public function perimeter() {
        return 2 * (this.width + this.height);
    }
}

function main() {
    Rectangle rect = new Rectangle(5.0, 3.0);
    print("Area: " + rect.area());
    print("Perimeter: " + rect.perimeter());
    return 0;
}
```

## 📚 Documentation

Full documentation is being developed. For now, refer to:
- [Language Specification](docs/language-spec.md) (Coming Soon)
- [API Reference](docs/api-reference.md) (Coming Soon)
- [Examples](examples/) (Coming Soon)

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by TypeScript, Java, and modern language design principles
- Built with C++17 for performance and reliability
- Special thanks to all contributors

## 📧 Contact

- Project Link: [https://github.com/yourusername/mType](https://github.com/yourusername/mType)
- Issues: [GitHub Issues](https://github.com/yourusername/mType/issues)

---

**mType** - A modern programming language for the future 🚀
