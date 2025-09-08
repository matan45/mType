# mType Language Project

## Overview
mType is a modern programming language implementation with a focus on type safety, object-oriented programming, and performance optimization. The project includes a complete interpreter with lexical analysis, parsing, semantic analysis, and execution capabilities.

## Features

### Core Language Features
- **Static Type System**: Strong type checking with compile-time error detection
- **Object-Oriented Programming**: Full support for classes, inheritance, encapsulation
- **Namespace Support**: Hierarchical organization of code with nested namespaces
- **Function Overloading**: Multiple functions with the same name but different signatures
- **Control Flow**: Complete set of control structures (if/else, loops, switch statements)
- **Memory Management**: Automatic memory management with garbage collection
- **Import System**: Module-based code organization with dependency resolution
- **Static Members**: Class-level variables and methods
- **Final Variables**: Immutable variable declarations
- **Method Chaining**: Fluent interface support with `this` returns

### Advanced Features
- **Collection Framework**: Generic collections (Array, Set, Map, Stack, Queue) with type safety
- **Content-Based Object Hashing**: Objects in collections are compared by field values, not identity
- **Recursive Nested Object Support**: Collections handle nested objects with deep content comparison
- **Optimization Levels**: `-O2` flag for performance optimization
- **Error Recovery**: Robust error handling with detailed error messages
- **Testing Framework**: Built-in test runner with comprehensive test suites
- **Circular Import Detection**: Prevention of circular dependencies
- **Type Inference**: Automatic type deduction where possible

## Project Structure

```
mType/
├── mType/                          # Core interpreter source
│   ├── src/
│   │   ├── lexer/                  # Lexical analysis
│   │   │   ├── Lexer.cpp
│   │   │   └── Lexer.h
│   │   ├── parser/                 # Syntax analysis
│   │   │   ├── Parser.cpp
│   │   │   └── Parser.h
│   │   ├── semantic/               # Semantic analysis
│   │   │   ├── SemanticAnalyzer.cpp
│   │   │   └── SemanticAnalyzer.h
│   │   ├── interpreter/            # Runtime execution
│   │   │   ├── Interpreter.cpp
│   │   │   └── Interpreter.h
│   │   ├── ast/                    # Abstract Syntax Tree
│   │   │   ├── ASTNode.cpp
│   │   │   └── ASTNode.h
│   │   ├── environment/            # Variable scoping
│   │   │   ├── Environment.cpp
│   │   │   └── Environment.h
│   │   ├── types/                  # Type system
│   │   │   ├── TypeChecker.cpp
│   │   │   └── TypeChecker.h
│   │   └── utils/                  # Utility functions
│   │       ├── ErrorReporter.cpp
│   │       └── ErrorReporter.h
│   ├── tests/                      # Test files
│   │   ├── testFiles/
│   │   │   ├── class/              # Class-related tests
│   │   │   ├── controlFlow/        # Control flow tests
│   │   │   ├── nameSpace/          # Namespace tests
│   │   │   ├── import/             # Import system tests
│   │   │   ├── integration/        # Integration tests
│   │   │   ├── typeChecking/       # Type checking tests
│   │   │   └── collections/        # Collection framework tests
│   │   └── TestRunner.cpp
│   └── mType.vcxproj              # Visual Studio project file
├── bin/                           # Compiled executables
│   └── mType/Debug/x64/
│       └── mType.exe
├── Interpreter.sln                # Visual Studio solution
└── CLAUDE.md                     # This documentation file
```

## Coding Style Guidelines

### Naming Conventions
- **Classes**: PascalCase (e.g., `SemanticAnalyzer`, `ASTNode`)
- **Functions/Methods**: camelCase (e.g., `parseExpression`, `evaluateNode`)
- **Variables**: camelCase (e.g., `currentToken`, `symbolTable`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `MAX_RECURSION_DEPTH`)
- **Namespaces**: lowercase (e.g., `mtype`, `utils`)

### File Organization
- **Header Files**: `.h` extension with include guards or `#pragma once`
- **Implementation Files**: `.cpp` extension
- **One Class per File**: Each major class should have its own header/implementation pair
- **Forward Declarations**: Use forward declarations to minimize header dependencies

### Code Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**: Opening brace on same line for control structures, new line for functions/classes
- **Line Length**: Maximum 120 characters per line
- **Spacing**: Space around operators, after commas, before opening parentheses in control structures

### Documentation
- **Header Comments**: Brief description of class/function purpose
- **Inline Comments**: Explain complex algorithms or business logic
- **Parameter Documentation**: Document function parameters and return values

## SOLID Principles Implementation

### Single Responsibility Principle (SRP)
- **Lexer**: Solely responsible for tokenizing input
- **Parser**: Only handles syntax analysis and AST construction
- **SemanticAnalyzer**: Focused exclusively on type checking and semantic validation
- **Interpreter**: Dedicated to runtime execution
- **ErrorReporter**: Handles all error reporting and formatting

### Open/Closed Principle (OCP)
- **ASTNode Hierarchy**: Base `ASTNode` class with virtual methods, extensible through inheritance
- **Visitor Pattern**: Used for AST traversal, allowing new operations without modifying existing nodes
- **Strategy Pattern**: Different optimization strategies can be plugged in without changing core logic

### Liskov Substitution Principle (LSP)
- **AST Nodes**: All derived node types can be used interchangeably through base class pointers
- **Type System**: Derived types maintain the contract of their base types
- **Polymorphic Behavior**: Virtual functions ensure correct behavior regardless of actual object type

### Interface Segregation Principle (ISP)
- **Specialized Interfaces**: Separate interfaces for different concerns (e.g., `Visitable`, `Evaluable`)
- **Minimal Dependencies**: Classes only depend on interfaces they actually use
- **Role-Based Interfaces**: Different roles have different interface requirements

### Dependency Inversion Principle (DIP)
- **Abstract Dependencies**: High-level modules depend on abstractions, not concretions
- **Dependency Injection**: Pass dependencies through constructors or parameters
- **Interface-Based Design**: Core components interact through well-defined interfaces

## Modern C++ Features

### C++17/20 Features Used
- **Smart Pointers**: `std::unique_ptr`, `std::shared_ptr` for automatic memory management
- **Auto Keyword**: Type deduction for complex template types
- **Range-Based Loops**: Modern iteration syntax
- **Lambda Expressions**: For callbacks and functional programming constructs
- **Move Semantics**: Efficient resource transfer with move constructors/operators
- **Variadic Templates**: For flexible function signatures

### Memory Management
- **RAII**: Resource Acquisition Is Initialization for automatic cleanup
- **Smart Pointers**: Preferred over raw pointers for ownership management
- **Container Classes**: STL containers for automatic memory management
- **Exception Safety**: Strong exception safety guarantees where possible

### Performance Considerations
- **Move Semantics**: Minimize unnecessary copies
- **Reserve Capacity**: Pre-allocate container sizes when known
- **Const Correctness**: Use const wherever possible for optimization hints
- **Inline Functions**: Hot path functions marked inline
- **Optimization Flags**: Support for `-O2` optimization level

## Build and Test Commands

### Building the Project
```bash
# Build with MSBuild
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Interpreter.sln /p:Configuration=Debug /p:Platform=x64

# Clean build
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Interpreter.sln /t:Clean
```

### Running Tests
```bash
# Run all tests
"bin\mType\Debug\x64\mType.exe" --tests

# Run specific test categories
"bin\mType\Debug\x64\mType.exe" --test class
"bin\mType\Debug\x64\mType.exe" --test namespace
"bin\mType\Debug\x64\mType.exe" --test import
"bin\mType\Debug\x64\mType.exe" --test collections

# Run with optimization
"bin\mType\Debug\x64\mType.exe" -O2 --tests
```

### Running Individual Files
```bash
# Basic execution
"bin\mType\Debug\x64\mType.exe" "path/to/file.mt"

# With optimization
"bin\mType\Debug\x64\mType.exe" -O2 "path/to/file.mt"
```

## Development Workflow

1. **Write Tests First**: Follow TDD approach where possible
2. **Incremental Development**: Small, focused commits
3. **Code Review**: All changes should be reviewed
4. **Documentation**: Update documentation with new features
5. **Performance Testing**: Benchmark critical paths
6. **Memory Profiling**: Check for leaks and inefficient allocations

## Collection Framework

### Supported Collection Types
- **Array<T>**: Dynamic resizable arrays with indexed access
- **Set<T>**: Unique element collections with content-based hashing
- **Map<K,V>**: Key-value pairs with efficient lookup
- **Stack<T>**: LIFO (Last In, First Out) operations
- **Queue<T>**: FIFO (First In, First Out) operations

### Collection Features
- **Generic Type Safety**: All collections are strongly typed with template support
- **Content-Based Object Equality**: Objects are compared by field values, not memory addresses
- **Recursive Nested Object Support**: Deep comparison for objects containing other objects
- **Comprehensive API**: Full set of operations (add, remove, contains, size, iteration)
- **For-Each Loop Support**: All collections support `foreach` syntax
- **Memory Management**: Automatic cleanup with RAII principles

### Object Hashing Algorithm
Objects in collections use content-based hashing:
1. **Class Name**: Contributes to hash uniqueness
2. **Field Values**: All instance fields included in sorted order
3. **Nested Objects**: Recursive content hashing for object fields
4. **Primitive Types**: Direct value inclusion (int, float, string, bool)
5. **Collections as Fields**: Reference-based hashing to avoid circular dependencies

### Example Usage
```cpp
// Type-safe collections
Set<Person> people = new Set<Person>();
Map<string, Account> accounts = new Map<string, Account>();

// Content-based object comparison
Person alice1 = new Person("Alice", 25);
Person alice2 = new Person("Alice", 25); // Same content
people.add(alice1); // true
people.add(alice2); // false - duplicate content detected

// For-each iteration
foreach (Person p in people) {
    print(p.getName());
}
```

## Contributing Guidelines

- Follow the established coding style consistently
- Write comprehensive tests for new features
- Maintain backward compatibility where possible
- Document public APIs thoroughly
- Consider performance implications of changes
- Use modern C++ features appropriately
- Test collection operations with various object types
- Verify content-based hashing works correctly for nested structures