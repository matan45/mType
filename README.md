<div align="center">
  <img src="logo.png" alt="mType Logo" width="400"/>
</div>

# mType Programming Language

![Version](https://img.shields.io/badge/version-0.2.0-blue)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)

A modern, statically-typed programming language with a bytecode virtual machine, featuring object-oriented programming, generics, lambdas, async/await, and a comprehensive type system inspired by TypeScript and Java.

## Key Highlights

- **Bytecode VM Architecture**: Stack-based virtual machine with 130+ optimized opcodes
- **Advanced Type System**: Full generic support with type constraints and inference
- **Modern Features**: Async/await, lambdas, closures, and functional programming
- **Object-Oriented**: Classes, interfaces, inheritance, and polymorphism
- **Performance Optimized**: SIMD-accelerated operations, string pooling, and AST optimizations
- **Full IDE Support**: VS Code extension with IntelliSense, debugging, and formatting
- **Standard Library**: Rich collection framework (List, HashMap, Stack, Queue, etc.)

## 🌟 Feature Overview

### Core Language Features
- **Static Typing**: `int`, `float`, `string`, `bool`, `void`, and `null` types
- **Type Inference**: Automatic type deduction for variables and expressions
- **Modern Operators**: Arithmetic, comparison, logical, ternary, and compound assignment
- **Control Flow**: if/else, switch/case, for/while/do-while loops, break/continue
- **Multi-dimensional Arrays**: Jagged and rectangular arrays with SIMD operations
- **Exception Handling**: try/catch/finally with custom exception hierarchies

### Object-Oriented Programming
- **Classes**: Full class support with constructors and inheritance
- **Interfaces**: Multiple interface implementation and inheritance
- **Access Modifiers**: `public`, `private`, `protected`
- **Abstract Classes**: Abstract classes and methods
- **Final Modifiers**: Final classes and methods to prevent overriding
- **Static Members**: Class-level fields and methods
- **Polymorphism**: Dynamic dispatch and runtime type checking
- **Method Overriding**: With covariant return types

### Advanced Type System
- **Generics**: Generic classes, interfaces, and methods
  - Multiple type parameters: `class Pair<K, V>`
  - Type constraints: `class Box<T extends Comparable>`
  - Nested generics: `List<Map<String, Int>>`
- **Type Casting**: Safe downcasting with runtime validation
- **Nullable Types**: Explicit null handling
- **Runtime Type Checking**: `isClassOf` operator

### Functional Programming
- **Lambda Expressions**: `x -> x * 2` and `(x, y) -> { return x + y; }`
- **Closures**: Capture variables from outer scope
- **Functional Interfaces**: `Function<T,R>`, `Consumer<T>`, `Predicate<T>`, `Supplier<T>`
- **Higher-Order Functions**: Functions as parameters and return values

### Asynchronous Programming
- **async/await Syntax**: Modern asynchronous code
- **Promise<T> Type**: Built-in promise support
- **Event Loop**: Priority-based task scheduling
- **Async Lambdas**: Asynchronous lambda expressions

### Standard Library
- **Collections**:
  - `List<T>`: Dynamic resizable list
  - `LinkedList<T>`: Doubly-linked list
  - `Stack<T>`: LIFO stack
  - `Queue<T>`: FIFO queue
  - `HashMap<K,V>`: Hash-based map
  - `HashSet<T>`: Hash-based set
- **Primitive Wrappers**: `Int`, `Float`, `String`, `Bool`
- **Exception Types**: `Exception`, `RuntimeException`, `NullPointerException`, `IndexOutOfBoundsException`
- **Built-in Functions**: `print()`, `typeof()`, `hashCode()`, array operations

## 🚀 Bytecode VM & Compilation

### Architecture

mType v0.2.0 features a complete bytecode compilation pipeline:

```
Source Code (.mt)
    ↓ Lexer
Tokens
    ↓ Parser
Abstract Syntax Tree (AST)
    ↓ Optimizer (optional)
Optimized AST
    ↓ Bytecode Compiler
Bytecode Program (.mtc)
    ↓ Virtual Machine
Execution Result
```

### Execution Modes

**1. Direct Execution** (compile and run in memory)
```bash
mType script.mt
```

**2. Compile to Bytecode** (create .mtc file)
```bash
# Debug mode (minimal optimizations)
mType --compile script.mt

# Release mode (full optimizations)
mType --compile -release script.mt
```

**3. Execute Bytecode** (run compiled .mtc file)
```bash
mType script.mtc
```

### Virtual Machine Features

- **Stack-Based Architecture**: Fast execution with 130+ specialized opcodes
- **Call Stack Protection**: Configurable stack depth with overflow detection (max: 1000)
- **Type-Specialized Instructions**: Optimized int/float operations
- **Fast Field Access**: Cached offset-based field access
- **Debugging Support**: Breakpoints, line tracking, stack traces
- **Event Loop Integration**: For async/await execution

### Optimization System

**AST-Level Optimizations** (enabled with `-release` flag):

1. **Constant Folding**
   - Compile-time evaluation of constant expressions
   - Arithmetic, logical, and comparison operations
   - String concatenation
   - Overflow/underflow detection

2. **Dead Code Elimination**
   - Removes unreachable code after return/break/continue/throw
   - Respects `@Script` and `@Throw` annotations
   - O(n) sequential block walking

3. **Unused Declaration Elimination**
   - Two-phase analysis: usage tracking + removal
   - Preserves public/exported declarations
   - Maintains entry point code

## ⚡ Memory Management & Performance

### String Pooling (Flyweight Pattern)
- **Automatic Deduplication**: Intern strings to save memory
- **LRU Cleanup**: Automatic cleanup with configurable limits
- **Thread-Safe**: Concurrent access support
- **Statistics Tracking**: Monitor pool hits and memory savings
- **Configuration**: Max 100,000 strings, 1-1024 char length range

### Array Pooling (Object Pooling)
- **Dimension-Based Reuse**: Pool common array sizes (powers of 2, common 2D)
- **Hit Rate Tracking**: Monitor pool efficiency
- **Adaptive Storage**: Automatic sparse vs. dense selection
- **Memory Limits**: Max 1M elements total

### SIMD-Accelerated Operations
Hardware-accelerated array operations (3-8× faster for arrays ≥16 elements):
- **Platform Support**: SSE2, AVX2 (x86/x64), NEON (ARM)
- **Operations**: Addition, subtraction, multiplication, scalar operations
- **Reductions**: Sum, min, max, average
- **Runtime Detection**: Automatic CPU feature detection

### Structure-of-Arrays (SoA) Optimization
- **95% Memory Reduction**: For large object arrays
- **Cache-Friendly Layout**: Contiguous field storage
- **SIMD-Optimized**: Field-oriented memory layout
- **Sub-Array Views**: Zero-copy array slicing

## 🛠️ VS Code Extension

Full-featured IDE support with the **mType Language Extension**:

### Code Intelligence
- **IntelliSense**: Context-aware completions (100+ keywords, types, members)
- **Signature Help**: Real-time parameter hints for functions and methods
- **Go-to-Definition** (F12): Jump to class, method, field, variable definitions
- **Find All References** (Shift+F12): Workspace-wide reference search
- **Code Lens**: Reference counts above classes and methods

### Code Quality
- **Auto-Formatting** (Shift+Alt+F): AST-based code formatting
- **Quick Fixes**: Auto-import, interface implementation, organize imports
- **Import Management**: Path resolution and broken import diagnostics
- **Semantic Highlighting**: Enhanced token-based syntax coloring

### Debugging
- **Breakpoints**: Set breakpoints in .mt source files
- **Step Execution**: Step over, step into, step out
- **Variable Inspection**: View variables in debug sidebar
- **Call Stack Navigation**: Inspect call hierarchy
- **Exception Breakpoints**: Break on all/uncaught exceptions

### Theming & Visual
- **Syntax Highlighting**: Comprehensive TextMate grammar (50+ token categories)
- **Color Themes**: mType Dark (Atom One Dark-inspired) and Light themes
- **Custom Icons**: Distinct icons for .mt and .mtc files
- **Bracket Matching**: Auto-closing and smart indentation

### Configuration Options
- Tab size, use tabs/spaces
- Format on save
- Organize imports automatically
- Space around operators
- Blank lines between declarations

## 📦 Getting Started

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Premake5 (for project generation)

### Building from Source

#### Windows (Visual Studio)
```bash
git clone https://github.com/matan45/mType.git
cd mType
runPremake.bat
# Open Interpreter.sln in Visual Studio and build
```

#### Linux/macOS
```bash
git clone https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

### Installing VS Code Extension

1. Open VS Code
2. Go to Extensions (Ctrl+Shift+X)
3. Search for "mType" or install from `mtype-vscode-extension/`
4. Reload VS Code

### Running Your First Program

Create `hello.mt`:
```mtype
function main() {
    string greeting = "Hello, mType!";
    print(greeting);
    return 0;
}
```

Run it:
```bash
# Direct execution
mType hello.mt

# Compile to bytecode (optimized)
mType --compile -release hello.mt

# Execute bytecode
mType hello.mtc
```

## 📁 Project Structure

```
mType/
├── lib/                          # Standard library
│   ├── Object.mt
│   ├── collections/              # List, HashMap, Stack, Queue, etc.
│   └── primitives/               # Int, Float, String, Bool wrappers
├── mType/                        # Core interpreter
│   ├── ast/                      # Abstract Syntax Tree nodes
│   ├── lexer/                    # Lexical analysis
│   ├── parser/                   # Syntax analysis
│   ├── optimizer/                # AST optimization passes
│   ├── vm/                       # Virtual Machine
│   │   ├── bytecode/             # Bytecode definitions and serialization
│   │   ├── compiler/             # Bytecode compiler
│   │   └── runtime/              # VM execution engine
│   ├── environment/              # Variable scoping and registries
│   ├── types/                    # Type system
│   ├── value/                    # Value types and memory management
│   ├── services/                 # File reading, imports, script execution
│   ├── errors/                   # Exception definitions
│   └── tests/                    # Comprehensive test suite
│       └── testFiles/            # Test cases by feature
├── mtype-vscode-extension/       # VS Code language support
│   ├── src/                      # Extension implementation
│   │   ├── completion/           # IntelliSense
│   │   ├── definition/           # Go-to-definition
│   │   ├── references/           # Find references
│   │   ├── formatter/            # Code formatting
│   │   └── debug/                # Debug adapter
│   ├── syntaxes/                 # Syntax highlighting
│   └── themes/                   # Color themes
└── bin/                          # Compiled executables
```

## 📝 Code Examples

### Generics

```mtype
class Box<T> {
    private T value;

    public constructor(T val) {
        this.value = val;
    }

    public function get(): T {
        return this.value;
    }

    public function set(T val): void {
        this.value = val;
    }
}

function main():void {
    Box<int> intBox = new Box<int>(42);
    Box<string> strBox = new Box<string>("Hello");

    print(intBox.get());    // 42
    print(strBox.get());    // Hello
    return 0;
}
```

### Lambdas & Functional Programming

```mtype
interface Function<T, R> {
    function apply(T arg): R;
}

function map<T, R>(T[] array, Function<T, R> mapper): R[] {
    R[] result = new R[array.length];
    for (int i = 0; i < array.length; i++) {
        result[i] = mapper.apply(array[i]);
    }
    return result;
}

function main():int {
    int[] numbers = [1, 2, 3, 4, 5];

    // Lambda expression
    Function<int, int> doubler = x -> x * 2;
    int[] doubled = map(numbers, doubler);

    // Inline lambda
    int[] squared = map(numbers, x -> x * x);

    print(doubled);  // [2, 4, 6, 8, 10]
    print(squared);  // [1, 4, 9, 16, 25]
    return 0;
}
```

### Async/Await

```mtype
function async fetchData(string url): Promise<string> {
    // Simulate async operation
    return "Data from " + url;
}

function async processData(): Promise<void> {
    string data1 = await fetchData("https://api.example.com/users");
    print("Received: " + data1);

    string data2 = await fetchData("https://api.example.com/posts");
    print("Received: " + data2);
}

function main(): void {
    processData();
    return 0;
}
```

### Interfaces & Polymorphism

```mtype
interface Drawable {
    function draw(): string;
}

interface Resizable {
    function resize(float factor): void;
}

class Circle implements Drawable, Resizable {
    private float radius;

    public constructor(float r) {
        this.radius = r;
    }

	@Override
    public function draw(): string {
        return "Drawing circle with radius " + parsePrimitive(this.radius);
    }
	
	@Override
    public function resize(float factor) {
        this.radius = this.radius * factor;
    }
}

function main(): void {
    Circle circle = new Circle(5.0);
    print(circle.draw());     // Drawing circle with radius 5.0

    circle.resize(2.0);
    print(circle.draw());     // Drawing circle with radius 10.0

    return 0;
}
```

### Class Inheritance

```mtype
class Animal {
    protected string name;

    public constructor(string n) {
        this.name = n;
    }

    public function speak(): string {
        return "Some sound";
    }
}

class Dog extends Animal {
    public constructor(string n):super(n) {
    }

    @Override
    public function speak(): string {
        return "Woof! I'm " + this.name;
    }
}

function main(): void {
    Animal animal = new Dog("Buddy");
    print(animal.speak());  // Woof! I'm Buddy (polymorphism)
    return 0;
}
```

### Collections

```mtype
import { List, HashMap } from "lib/collections";

function main(): void {
    // List example
    List<int> numbers = new List<int>();
    numbers.add(10);
    numbers.add(20);
    numbers.add(30);
    print(numbers.size());  // 3

    // HashMap example
    HashMap<string, int> ages = new HashMap<string, int>();
    ages.put("Alice", 30);
    ages.put("Bob", 25);

    if (ages.containsKey("Alice")) {
        print(ages.get("Alice"));  // 30
    }

    return 0;
}
```

## 📊 Current Status (v0.2.0)

### Completed
- ✅ Lexical Analysis (Lexer with full token support)
- ✅ Syntax Analysis (Parser with error recovery)
- ✅ Abstract Syntax Tree (50+ node types)
- ✅ Semantic Analysis (Type checking, scope analysis)
- ✅ Generic Type System (Full generic support with constraints)
- ✅ Bytecode Compiler (130+ opcodes, serialization)
- ✅ Virtual Machine (Stack-based execution engine)
- ✅ Optimization System (3 AST optimization passes)
- ✅ Memory Management (String pooling, array pooling, SoA)
- ✅ Standard Library (Collections, primitives, exceptions)
- ✅ Async/Await (Promise support with event loop)
- ✅ Exception Handling (try/catch/finally)
- ✅ Import System (With circular dependency detection)
- ✅ VS Code Extension (Full IDE support)
- ✅ Debugging Support (Debug adapter protocol)
- ✅ SIMD Acceleration (SSE2, AVX2, NEON)

### In Progress
- 🚧 Standard Library Expansion
- 🚧 Performance Benchmarking Suite
- 🚧 Language Server Protocol (LSP)
- 🚧 Documentation Portal

### Planned
- 📋 JIT Compilation
- 📋 Package Manager
- 📋 REPL (Read-Eval-Print Loop)
- 📋 Native Code Generation
- 📋 Cross-Platform Distribution

## 🔧 Technical Details

### C++17/20 Modern Features
- **Smart Pointers**: `std::unique_ptr`, `std::shared_ptr` for automatic memory management
- **Move Semantics**: Efficient resource transfer
- **Lambda Expressions**: For callbacks and functional constructs
- **RAII**: Resource Acquisition Is Initialization
- **Variadic Templates**: Flexible function signatures
- **Range-Based Loops**: Modern iteration syntax

### Design Principles
- **SOLID Principles**: Single Responsibility, Open/Closed, Liskov Substitution, Interface Segregation, Dependency Inversion
- **DRY**: Don't Repeat Yourself
- **KISS**: Keep It Simple, Stupid
- **Modular Architecture**: Separated concerns with clear interfaces
- **Code Quality**: 50-line function limit, extensive test coverage

### Performance Characteristics
- **Bytecode Execution**: Faster than tree-walking interpretation
- **Optimized Instructions**: Type-specialized opcodes
- **Memory Efficiency**: String interning, array pooling, SoA storage
- **SIMD Operations**: 3-8× speedup for array operations
- **Stack-Based VM**: Minimal overhead for function calls

## 🧪 Testing

### Running Tests

The test suite is built into the interpreter:

```bash
# Build and run all tests
mType

# Run specific test suite
mType --test arrays
mType --test class
mType --test generics

# Verbose output
mType --test --verbose
```

### Test Coverage

Comprehensive test suites covering:
- ✅ Arrays (single, multi-dimensional, jagged, SIMD)
- ✅ Classes (inheritance, constructors, polymorphism)
- ✅ Interfaces (single, multiple, inheritance)
- ✅ Generics (classes, methods, constraints)
- ✅ Lambdas (expression, block, closures)
- ✅ Control Flow (if/else, loops, switch, break/continue)
- ✅ Type Checking (casting, validation, inference)
- ✅ Exception Handling (try/catch/finally, custom exceptions)
- ✅ Imports (selective, wildcard, circular detection)
- ✅ Async/Await (promises, event loop)
- ✅ String Pooling (interning, deduplication)
- ✅ Collections (List, HashMap, Stack, Queue)

## 🤝 Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Follow C++17 standards and SOLID principles
4. Write tests for new features
5. Ensure 50-line function limit
6. Update documentation
7. Commit your changes (`git commit -m 'Add AmazingFeature'`)
8. Push to the branch (`git push origin feature/AmazingFeature`)
9. Open a Pull Request

### Development Guidelines
- Use smart pointers for memory management
- Follow the Result<T> pattern for error handling
- Maintain modular architecture with clear separation of concerns
- Write comprehensive test cases
- Document complex algorithms and design decisions

## 📚 Documentation

- [Language Specification](docs/language-spec.md) (Coming Soon)
- [Bytecode Reference](docs/bytecode-reference.md) (Coming Soon)
- [Standard Library API](docs/stdlib-api.md) (Coming Soon)
- [VM Architecture](docs/vm-architecture.md) (Coming Soon)

For detailed project structure and implementation details, see [CLAUDE.md](CLAUDE.md).

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by TypeScript, Java, and modern language design principles
- Built with C++17 for performance and reliability
- Special thanks to all contributors and the open-source community

## 📧 Contact

- Project Repository: [https://github.com/matan45/mType](https://github.com/matan45/mType)
- Issues: [GitHub Issues](https://github.com/matan45/mType/issues)
- Discussions: [GitHub Discussions](https://github.com/matan45/mType/discussions)

---

**mType v0.2.0** - A modern programming language with bytecode VM, generics, async/await, and comprehensive tooling.
