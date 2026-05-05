<div align="center">
  <img src="logo.png" alt="mType Logo" width="400"/>
</div>

# mType Programming Language

![Version](https://img.shields.io/badge/version-0.2.0-blue)
[![CI](https://github.com/matan45/mType/actions/workflows/ci.yml/badge.svg)](https://github.com/matan45/mType/actions/workflows/ci.yml)
![License](https://img.shields.io/badge/license-MIT-green)
![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)

A modern, statically-typed programming language with a bytecode virtual machine, featuring object-oriented programming, generics, lambdas, async/await, and a comprehensive type system inspired by TypeScript and Java.

## Key Highlights

- **Bytecode VM + JIT Compilation**: Stack-based VM with asmjit-powered x86-64 JIT compiler
- **Advanced Type System**: Full generic support with type constraints and inference
- **Modern Features**: Async/await, lambdas, closures, and functional programming
- **Object-Oriented**: Classes, interfaces, inheritance, polymorphism, and value types
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
- **Value Classes**: Lightweight value types with copy semantics and structural equality
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
- **Null Safety**: Non-nullable by default, explicit `?` suffix for nullable types
  - Non-nullable: `string name = "hello"` (cannot be null)
  - Nullable: `string? name = null` (explicitly allows null)
  - Smart casts: Automatic narrowing after null checks
  - Compile-time enforcement: Assigning `null` to non-nullable types is a compile error
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
- **Primitive Wrappers**: `Int`, `Float`, `String`, `Bool` (value classes with minimal overhead)
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
    ↓   ↓ Hot functions detected by profiler
    ↓   ↓ JIT Compiler (asmjit x86-64)
    ↓   ↓ Native Machine Code
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

### Project System (`.mtproj`)

mType includes a built-in project system for managing multi-file projects using XML-based `.mtproj` configuration files.

**Initialize a new project:**
```bash
mType --init MyApp src/**/*.mt
```

This creates a `MyApp.mtproj` file:
```xml
<Project Name="MyApp" Version="1.0.0">
  <Source>
    <Include>src/**/*.mt</Include>
  </Source>
  <Output Directory="build" />
  <Imports>
  </Imports>
</Project>
```

**Build the project:**
```bash
# Compile all source files to bytecode
mType --build

# Build into a single .mtcLib library file
mType --build --lib

# Specify project file explicitly
mType --build MyApp.mtproj
```

**Manage source files:**
```bash
# Add include pattern
mType --add "tests/**/*.mt"

# Remove include pattern
mType --remove "tests/**/*.mt"

# Clean build artifacts
mType --clean
```

**Project features:**
- **Auto-discovery**: Walks up directory tree to find the nearest `.mtproj` file
- **Glob patterns**: Include/exclude files using `**/*.mt` glob syntax
- **Import paths**: Configure search paths and aliases for module resolution
- **Library builds**: Compile into a single `.mtcLib` distributable file
- **Progress reporting**: Real-time compilation progress with file counts

```xml
<Project Name="MyApp" Version="1.0.0">
  <Source>
    <Include>src/**/*.mt</Include>
    <Include>lib/**/*.mt</Include>
    <Exclude>src/tests/**/*.mt</Exclude>
  </Source>
  <Output Directory="build" />
  <Imports>
    <SearchPath>lib</SearchPath>
    <Alias Name="utils" Path="src/utils" />
  </Imports>
</Project>
```

### Virtual Machine Features

- **Stack-Based Architecture**: Fast execution with 130+ specialized opcodes
- **JIT Compilation**: Automatic compilation of hot functions to native x86-64 code via [asmjit](https://github.com/asmjit/asmjit)
  - Profile-guided: functions are JIT-compiled after becoming hot
  - On-Stack Replacement (OSR): hot loops are compiled mid-execution
  - Inline Caching (IC): monomorphic/polymorphic field access optimization
  - Typed array fast paths: direct primitive access for `int[]`/`float[]`
  - SoA-aware array field access: column-oriented storage for object arrays
- **Call Stack Protection**: Configurable stack depth with overflow detection (max: 1000)
- **Type-Specialized Instructions**: Optimized int/float operations
- **Fast Field Access**: Inline-cached offset-based field access
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

### Value Object Optimization
- **Lightweight Runtime**: Value classes use indexed `vector<Value>` fields (~48-80 bytes) instead of hash-map-based `ObjectInstance` (~230+ bytes)
- **No GC Registration**: Value objects use reference counting via `shared_ptr`, avoiding GC overhead
- **Copy Semantics**: Deep copy on assignment prevents shared-state bugs
- **Structural Equality**: Field-by-field comparison without custom `equals()` methods

### Structure-of-Arrays (SoA) Optimization
- **95% Memory Reduction**: For large object arrays
- **Cache-Friendly Layout**: Contiguous field storage
- **SIMD-Optimized**: Field-oriented memory layout
- **Sub-Array Views**: Zero-copy array slicing

## 📊 Benchmarking

Measure interpreter performance with the built-in benchmark suite:

```bash
# Run all 5 canonical benchmarks with 1 warmup + 3 measured iterations each
mType --benchmark

# Run a single benchmark script
mType --benchmark=mType/tests/testFiles/benchmarks/recursive.mt

# Disable JIT to capture a pure-interpreter baseline
mType --benchmark --no-jit

# JSON output (for CI diffing)
mType --benchmark --benchmark-output=json
```

The suite reports wall-clock time plus `ExecutionStats` (instructions,
function calls, string/array pool deltas) for:
`arithmetic_tight_loop`, `method_dispatch`, `object_alloc`, `string_ops`,
`recursive`.

See [`docs/benchmarks.md`](docs/benchmarks.md) for flags, interpreting
output, and updating the committed baseline at
[`docs/bench-baseline.md`](docs/bench-baseline.md).

## 🛠️ Editor & IDE Support

### Language Server Protocol (LSP)

mType now includes a **Language Server** that works with ANY editor supporting LSP!

**Supported Editors:**
- ✅ VS Code (via mType extension)
- ✅ Vim/Neovim (via coc.nvim or nvim-lspconfig)
- ✅ Emacs (via lsp-mode)
- ✅ Sublime Text (via LSP package)
- ✅ IntelliJ IDEA, Eclipse, and any LSP-compatible editor

**LSP Features:**
- Auto-completion (keywords, types, variables, built-ins)
- Hover information (type docs, signatures)
- Real-time diagnostics (errors and warnings)
- Future: Go-to-definition, find references, formatting

**Setup:** See [`languageserver/README.md`](languageserver/README.md) for installation and editor configuration.

### VS Code Extension

Full-featured IDE support with the **mType Language Extension**:

**Two Modes:**
1. **Built-in Mode** (default): VS Code-specific providers
2. **LSP Mode**: Universal language server (enable in settings)

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
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Premake5 (for project generation)

### Building from Source

#### Windows (Visual Studio)
```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
runPremake.bat
# Open Interpreter.sln in Visual Studio and build
```

#### Linux/macOS
```bash
git clone --recursive https://github.com/matan45/mType.git
cd mType
premake5 gmake2
make
```

> **Note:** If you cloned without `--recursive`, initialize the asmjit submodule:
> ```bash
> git submodule update --init --recursive
> ```

> **⚠️ After pulling a branch that changed `mType/vm/bytecode/OpCode.hpp`**
> (new opcode added / reordered / sentinel moved), do a **clean rebuild** —
> incremental builds can leave stale .obj/.lib artifacts compiled against
> the old enum layout, causing cross-TU dispatch drift that silently
> corrupts bytecode deserialization and opcode dispatch (symptom:
> "Unimplemented opcode: …" for handled opcodes, or "invalid opcode N"
> for opcodes that should be valid). See MYT-139.
>
> ```bash
> # Windows
> rmdir /s /q mType\obj mType\bin
> runPremake.bat
> # then Rebuild in Visual Studio
>
> # Linux/macOS
> rm -rf mType/obj mType/bin
> premake5 gmake2
> make clean && make
> ```
>
> `runPremake.bat` also sweeps orphan flat-layout .obj files from any
> previous premake config on every run (one-time migration no-op once
> the tree is clean).

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
├── vendor/
│   └── asmjit/                   # JIT assembler library (git submodule)
├── mType/                        # Core interpreter
│   ├── ast/                      # Abstract Syntax Tree nodes
│   ├── lexer/                    # Lexical analysis
│   ├── parser/                   # Syntax analysis
│   ├── optimizer/                # AST optimization passes
│   ├── vm/                       # Virtual Machine
│   │   ├── bytecode/             # Bytecode definitions and serialization
│   │   ├── compiler/             # Bytecode compiler
│   │   ├── runtime/              # VM execution engine
│   │   └── jit/                  # JIT compiler (x86-64 via asmjit)
│   │       ├── ic/               # Inline cache types and table
│   │       ├── guards/           # Deoptimization handlers
│   │       └── codegen/          # OSR entry codegen
│   ├── environment/              # Variable scoping and registries
│   ├── project/                  # Project system (.mtproj parsing, building)
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

### Value Classes

```mtype
// Value classes have copy semantics and structural equality
value class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int { return this.x; }
    public function getY(): int { return this.y; }

    public function add(Point other): Point {
        return new Point(this.x + other.x, this.y + other.y);
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

function main(): void {
    Point p1 = new Point(1, 2);
    Point p2 = p1;           // Deep copy (not a reference)
    Point p3 = new Point(1, 2);

    print(p1 == p3);         // true (structural equality)
    print(p1.add(p2).toString()); // (2, 4)
}
```

Value classes are:
- **Lightweight**: ~48-80 bytes per instance vs ~230+ bytes for regular objects
- **Copy-on-assign**: Assignment creates an independent copy
- **Structurally equal**: `==` compares field values, not references
- **Implicitly final**: Cannot be extended or inherit from other classes
- **Interface-compatible**: Can implement interfaces
- **Generic-compatible**: Work seamlessly in `List<Point>`, `HashMap<String, Point>`, etc.

The built-in primitive wrappers (`Int`, `Float`, `Bool`, `String`) are value classes, making generic collections significantly more memory-efficient.

### Null Safety

```mtype
class Box {
    public int value;

    public constructor(int v) {
        this.value = v;
    }
}

// Nullable parameters and return types
function findBox(Box?[] boxes, int target): Box? {
    for (int i = 0; i < boxes.length; i++) {
        if (boxes[i] != null) {
            // Smart cast: boxes[i] is treated as non-null Box here
            if (boxes[i].value == target) {
                return boxes[i];
            }
        }
    }
    return null;  // Valid — return type is Box?
}

function main(): void {
    // Non-nullable by default — cannot assign null
    Box box = new Box(42);

    // Nullable types use the ? suffix
    Box? nullable = null;
    Box? alsoNullable = new Box(10);

    // Smart cast: after a null check, the type is narrowed
    if (nullable != null) {
        // 'nullable' is treated as non-null Box here
        print(nullable.value);
    }

    // Works with value classes too
    // value class Point { ... }
    // Point? optionalPoint = null;

    // Compile-time error: cannot assign null to non-nullable type
    // Box invalid = null;  // ERROR!
}
```

Null safety features:
- **Non-nullable by default**: All types reject `null` unless marked with `?`
- **Nullable types**: Append `?` to any type — `int?`, `string?`, `MyClass?`
- **Function signatures**: Parameters (`Box? param`) and return types (`function find(): Box?`) support nullable
- **Value classes**: Value types like `Point?` follow the same nullable rules
- **Smart casts**: After an `if (x != null)` check, the compiler narrows the type automatically
- **Compile-time errors**: Assigning `null` to a non-nullable variable is caught at compile time
- **JIT optimized**: Non-nullable receivers skip runtime null checks for field access and method calls

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
- ✅ Language Server Protocol (LSP) - Universal editor support (Vim, Emacs, Sublime, etc.)
- ✅ JIT Compilation (x86-64 via asmjit, profile-guided, OSR, inline caching)
- ✅ Value Types (value classes with copy semantics, structural equality, lightweight runtime)
- ✅ Null Safety (non-nullable by default, `?` suffix, smart casts, compile-time enforcement)
- ✅ Project System (`.mtproj` with glob patterns, library builds, auto-discovery)
- ✅ Performance Benchmarking Suite (`--benchmark` flag, 5 canonical scripts, committed baseline)

### In Progress
- 🚧 Standard Library Expansion
- 🚧 Documentation Portal
- 🚧 LSP: Additional Features (go-to-definition, find references, formatting)

### Planned
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
- ✅ Classes (inheritance, constructors, polymorphism, value classes)
- ✅ Interfaces (single, multiple, inheritance)
- ✅ Generics (classes, methods, constraints)
- ✅ Lambdas (expression, block, closures)
- ✅ Control Flow (if/else, loops, switch, break/continue)
- ✅ Type Checking (casting, validation, inference)
- ✅ Exception Handling (try/catch/finally, custom exceptions)
- ✅ Imports (selective, wildcard, circular detection)
- ✅ Async/Await (promises, event loop)
- ✅ String Pooling (interning, deduplication)
- ✅ Null Safety (nullable types, smart casts, compile-time checks)
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

**mType v0.2.0** - A modern programming language with bytecode VM, value types, generics, async/await, and comprehensive tooling.
