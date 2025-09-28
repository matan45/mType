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

## Language Features Detail

### Control Flow Elements

#### Loops
- **For Loops**: Traditional C-style for loops with initialization, condition, and increment
  ```mtype
  for (int i = 0; i < 10; i++) {
      print(i);
  }
  ```
- **While Loops**: Condition-based iteration
  ```mtype
  while (condition) {
      // loop body
  }
  ```
- **Do-While Loops**: Post-condition loops that execute at least once
  ```mtype
  do {
      // loop body
  } while (condition);
  ```
- **For-Each Loops**: Iterator-based loops for collections
  ```mtype
  foreach (item in collection) {
      print(item);
  }
  ```

#### Conditional Statements
- **If-Else Statements**: Basic conditional execution
  ```mtype
  if (condition) {
      // true branch
  } else if (otherCondition) {
      // else if branch
  } else {
      // else branch
  }
  ```
- **Switch-Case Statements**: Multi-way branching with default case
  ```mtype
  switch (expression) {
      case value1:
          // case 1 code
          break;
      case value2:
          // case 2 code
          break;
      default:
          // default code
  }
  ```
- **Ternary Operator**: Inline conditional expressions
  ```mtype
  result = condition ? trueValue : falseValue;
  ```

### Functions and Methods
- **Function Definitions**: Functions with parameters and return types
  ```mtype
  int add(int a, int b) {
      return a + b;
  }

  void printMessage(string msg) {
      print(msg);
  }
  ```
- **Method Overloading**: Multiple methods with same name but different signatures
- **Static Functions**: Class-level functions accessible without instantiation
- **Parameter Types**: Support for primitive types, objects, and generic types

### Type System and Type Checking
- **Static Type Checking**: Compile-time type validation with detailed error reporting
- **Type Inference**: Automatic type deduction where possible
- **Type Conversion**: Implicit and explicit type conversions with safety checks
- **Null Safety**: Null pointer protection with compile-time checks
- **Return Type Validation**: Ensures function return types match declarations
- **Parameter Type Matching**: Validates function call arguments against parameter types

### Arrays and Multi-Dimensional Arrays
- **1D Arrays**: Single-dimension arrays with dynamic sizing
  ```mtype
  int[] numbers = new int[10];
  string[] names = {"Alice", "Bob", "Charlie"};
  ```
- **Multi-Dimensional Arrays**: Support for 2D, 3D, and n-dimensional arrays
  ```mtype
  int[][] matrix = new int[3][3];
  int[][][] cube = new int[5][5][5];
  ```
- **Array Literals**: Direct array initialization with values
- **Dynamic Resizing**: Arrays can be resized at runtime
- **Bounds Checking**: Automatic prevention of out-of-bounds access
- **Generic Arrays**: Type-safe arrays with generic type parameters

### Import System
- **Simple Imports**: Import individual files or modules
  ```mtype
  import "math_utils.mt";
  import "string_utils.mt";
  ```
- **Namespace Imports**: Import specific namespaces
- **Circular Dependency Detection**: Prevents infinite import loops
- **Module Resolution**: Automatic resolution of import paths
- **Dependency Management**: Handles complex import hierarchies

### Classes and Object-Oriented Programming

#### Class Features
- **Class Definitions**: Full class support with fields and methods
  ```mtype
  class Person {
      private string name;
      private int age;

      public Person(string n, int a) {
          name = n;
          age = a;
      }

      public string getName() {
          return name;
      }
  }
  ```

#### Constructors
- **Default Constructors**: Parameterless constructors
- **Parameterized Constructors**: Constructors with parameters
- **Multiple Constructors**: Constructor overloading
- **Constructor Chaining**: Constructors calling other constructors

#### Static and Instance Members
- **Static Methods**: Class-level methods accessible without instances
  ```mtype
  static int compare(Person a, Person b) {
      return a.getAge() - b.getAge();
  }
  ```
- **Static Fields**: Class-level variables shared across all instances
- **Instance Methods**: Object-specific methods
- **Instance Fields**: Object-specific data members
- **Access Modifiers**: Public, private, and protected access control

#### Object Creation and Management
- **Object Instantiation**: Creating objects with `new` keyword
- **Member Access**: Accessing fields and methods with dot notation
- **Method Chaining**: Fluent interface support
- **Object Assignment**: Reference-based object assignment

### Interfaces

#### Interface Definition and Implementation
- **Interface Declarations**: Abstract contracts defining method signatures
  ```mtype
  interface Drawable {
      void draw();
      void setColor(string color);
  }
  ```
- **Interface Implementation**: Classes implementing interface contracts
  ```mtype
  class Circle implements Drawable {
      public void draw() {
          // implementation
      }

      public void setColor(string color) {
          // implementation
      }
  }
  ```

#### Interface Inheritance
- **Interface Extends**: Interfaces can extend other interfaces
  ```mtype
  interface ColoredDrawable extends Drawable {
      string getColor();
  }
  ```
- **Multiple Interface Implementation**: Classes can implement multiple interfaces
- **Interface Type Checking**: Compile-time validation of interface contracts

### Generics

#### Generic Classes
- **Generic Class Definitions**: Classes with type parameters
  ```mtype
  class Container<T> {
      private T value;

      public Container(T val) {
          value = val;
      }

      public T getValue() {
          return value;
      }
  }
  ```

#### Generic Interfaces
- **Generic Interface Definitions**: Interfaces with type parameters
  ```mtype
  interface Comparable<T> {
      int compareTo(T other);
  }
  ```

#### Generic Arrays
- **Generic Array Support**: Type-safe arrays with generic parameters
  ```mtype
  Container<string>[] containers = new Container<string>[10];
  ```

#### Generic Methods
- **Static Generic Methods**: Generic methods at class level
- **Instance Generic Methods**: Generic methods on objects
- **Type Parameter Constraints**: Constraining generic types

### String Pool Optimization
- **String Interning**: Automatic string pooling for memory efficiency
- **String Reuse**: Identical strings share memory locations
- **Performance Optimization**: Reduced memory allocation for strings
- **Garbage Collection**: Efficient cleanup of pooled strings

### Lambda Expressions and Functional Programming

#### Lambda Syntax
- **Expression Lambdas**: Single expression lambdas
  ```mtype
  (x, y) -> x + y
  ```
- **Block Lambdas**: Multi-statement lambda functions
  ```mtype
  (x) -> {
      print("Processing: " + x);
      return x * 2;
  }
  ```

#### Functional Interfaces
- **Lambda-Compatible Interfaces**: Interfaces with single abstract methods
- **Predicate Interface**: Boolean-returning functions
- **Consumer Interface**: Void-returning functions
- **Supplier Interface**: Parameter-less functions
- **Function Interface**: General-purpose function interface

#### Closure Support
- **Variable Capture**: Lambdas can capture local variables
- **Scope Management**: Proper handling of captured variable lifetimes
- **Memory Management**: Automatic cleanup of lambda contexts

### Integration Examples
The language supports complex integration scenarios combining multiple features:
- **Namespaces with Classes and Imports**: Complex module organization
- **Generic Collections with Lambda Processing**: Functional-style collection operations
- **Interface-based Design with Generic Constraints**: Advanced type system usage
- **Error Recovery with Partial Failures**: Robust error handling in complex scenarios
- **Memory Management with Deep Nesting**: Efficient resource management
- **Performance Optimization with Large Data Structures**: Scalable application development

All features are thoroughly tested with comprehensive test suites located in:
- `tests/testFiles/controlFlow/` - Control flow testing
- `tests/testFiles/typeChecking/` - Type system validation
- `tests/testFiles/arrays/` - Array functionality testing
- `tests/testFiles/import/` - Import system testing
- `tests/testFiles/class/` - Object-oriented features
- `tests/testFiles/interface/` - Interface system testing
- `tests/testFiles/generics/` - Generic type testing
- `tests/testFiles/stringpool/` - String optimization testing
- `tests/testFiles/lambda/` - Functional programming testing
- `tests/testFiles/integration/` - Complex integration scenarios
- `tests/testFiles/error/` - Error handling and edge cases

## Project Structure

```
mtype/
├── README.md
├── CLAUDE.md                      # This documentation file
├── Interpreter.sln               # Visual Studio solution
├── premake5.lua                  # Premake build configuration
├── rebuild-extension.bat         # Extension rebuild script
├── runPremake.bat               # Premake execution script
├── lib/                         # Standard library files
│   ├── Object.mt
│   ├── collections/             # Collection implementations
│   │   ├── HashMap.mt
│   │   ├── HashSet.mt
│   │   ├── LinkedList.mt
│   │   ├── List.mt
│   │   ├── Queue.mt
│   │   └── Stack.mt
│   └── primitives/              # Primitive type definitions
│       ├── Bool.mt
│       ├── Float.mt
│       ├── Int.mt
│       └── String.mt
├── mType/                       # Core interpreter source
│   ├── mType.vcxproj.filters
│   ├── ast/                     # Abstract Syntax Tree
│   │   ├── ASTNode.hpp
│   │   ├── ASTVisitor.hpp
│   │   ├── GenericType.cpp/.hpp
│   │   ├── GenericTypeParameter.cpp/.hpp
│   │   ├── NodeClassesDeclaration.hpp
│   │   └── nodes/               # AST Node implementations
│   │       ├── classes/         # Class-related nodes
│   │       ├── expressions/     # Expression nodes
│   │       ├── functions/       # Function nodes
│   │       └── statements/      # Statement nodes
│   ├── constants/               # Language constants
│   │   └── LambdaConstants.hpp
│   ├── environment/             # Variable scoping and environment
│   │   ├── Environment.cpp/.hpp
│   │   ├── EnvironmentBuilder.cpp/.hpp
│   │   ├── manager/             # Scope and variable management
│   │   └── registry/            # Class and function registries
│   ├── errors/                  # Error definitions
│   │   └── [Various exception types]
│   ├── evaluator/               # Runtime evaluation
│   │   ├── Evaluator.cpp/.hpp
│   │   ├── EvaluatorCoordinator.cpp/.hpp
│   │   ├── ExpressionEvaluator.cpp/.hpp
│   │   ├── ObjectEvaluator.cpp/.hpp
│   │   ├── StatementEvaluator.cpp/.hpp
│   │   ├── base/                # Evaluation context
│   │   ├── managers/            # Control flow and instance management
│   │   └── utils/               # Evaluation utilities
│   ├── exception/               # Control flow exceptions
│   │   ├── BreakException.hpp
│   │   ├── ContinueException.hpp
│   │   └── ReturnException.hpp
│   ├── exceptions/              # Dependency and error handling
│   │   ├── CircularDependencyDetection.cpp/.hpp
│   │   ├── CircularDependencyDetector.cpp/.hpp
│   │   └── DomainExceptions.hpp
│   ├── lexer/                   # Lexical analysis
│   │   ├── Lexer.cpp/.hpp
│   │   ├── BracketBalancer.cpp/.hpp
│   │   ├── SourceLocationTracker.cpp/.hpp
│   │   └── TokenFactory.cpp/.hpp
│   ├── parser/                  # Syntax analysis
│   │   ├── Parser.cpp/.hpp
│   │   ├── ClassParser.cpp/.hpp
│   │   ├── ExpressionParser.cpp/.hpp
│   │   ├── InterfaceParser.cpp/.hpp
│   │   ├── LambdaParser.cpp/.hpp
│   │   ├── StatementParser.cpp/.hpp
│   │   ├── TypeParser.cpp/.hpp
│   │   ├── ParseContext.cpp/.hpp
│   │   ├── ParserUtils.cpp/.hpp
│   │   ├── ParserValidator.cpp/.hpp
│   │   └── TokenStream.cpp/.hpp
│   ├── run/                     # Main entry point
│   │   └── Main.cpp
│   ├── runtimeTypes/            # Runtime type definitions
│   │   ├── Definition.cpp/.hpp
│   │   ├── collections/         # Collection type definitions
│   │   ├── global/              # Global function and variable definitions
│   │   └── klass/               # Class-related runtime types
│   ├── services/                # Core services
│   │   ├── FileReader.cpp/.hpp
│   │   ├── ImportManager.cpp/.hpp
│   │   └── ScriptInterpreter.cpp/.hpp
│   ├── tests/                   # Test framework and suites
│   │   ├── suites/              # Test suite implementations
│   │   ├── testFiles/           # Test case files
│   │   │   ├── arrays/          # Array tests (pass/error)
│   │   │   ├── class/           # Class tests (pass/error)
│   │   │   ├── controlFlow/     # Control flow tests (pass/error)
│   │   │   ├── error/           # Error handling tests
│   │   │   ├── generics/        # Generic type tests (pass/error)
│   │   │   ├── import/          # Import system tests (pass/error)
│   │   │   ├── integration/     # Integration tests
│   │   │   ├── interface/       # Interface tests (pass/error)
│   │   │   ├── lambda/          # Lambda tests (pass/error)
│   │   │   ├── lib/             # Library tests
│   │   │   ├── native/          # Native function tests
│   │   │   ├── stringpool/      # String pooling tests
│   │   │   └── typeChecking/    # Type checking tests (pass/error)
│   │   └── testFramework/       # Test framework implementation
│   ├── token/                   # Token definitions
│   │   ├── Token.hpp
│   │   └── TokenType.hpp
│   ├── types/                   # Type system
│   │   ├── TypeConversionUtils.cpp/.hpp
│   │   └── TypeRegistry.cpp/.hpp
│   └── value/                   # Value types and memory management
│       ├── ArrayPool.hpp
│       ├── FlatMultiArray.hpp
│       ├── LambdaValue.cpp/.hpp
│       ├── NativeArray.hpp
│       ├── ParameterType.hpp
│       ├── SparseMultiArray.hpp
│       ├── StringPool.cpp/.hpp
│       └── ValueType.hpp
├── mtype-vscode-extension/      # VS Code language extension
│   ├── README.md
│   ├── package.json
│   ├── tsconfig.json
│   ├── icons/                   # Extension icons
│   ├── language-configuration/  # Language configuration
│   ├── src/                     # Extension source code
│   │   ├── extension.ts
│   │   ├── analysis/            # Code analysis
│   │   ├── analyzer/            # Code analyzer
│   │   ├── completion/          # Auto-completion
│   │   ├── definition/          # Go-to-definition
│   │   ├── formatter/           # Code formatting
│   │   ├── imports/             # Import management
│   │   └── references/          # Find references
│   ├── syntaxes/               # Syntax highlighting
│   └── themes/                 # Color themes
└── bin/                        # Compiled executables
    └── mType/Debug/x64/
        └── mType.exe
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

## Implementation Mode and Design Principles

### SOLID Principles Implementation

#### Single Responsibility Principle (SRP)
- **Lexer**: Solely responsible for tokenizing input
- **Parser**: Only handles syntax analysis and AST construction
- **SemanticAnalyzer**: Focused exclusively on type checking and semantic validation
- **Evaluator**: Dedicated to runtime execution and interpretation
- **ErrorReporter**: Handles all error reporting and formatting
- **Environment**: Manages variable scoping and symbol tables
- **TypeRegistry**: Maintains type definitions and relationships

#### Open/Closed Principle (OCP)
- **ASTNode Hierarchy**: Base `ASTNode` class with virtual methods, extensible through inheritance
- **Visitor Pattern**: Used for AST traversal, allowing new operations without modifying existing nodes
- **Strategy Pattern**: Different optimization strategies can be plugged in without changing core logic
- **Plugin Architecture**: New language features can be added without modifying core components
- **Evaluator Extensions**: New evaluation strategies can be implemented without changing base evaluator

#### Liskov Substitution Principle (LSP)
- **AST Nodes**: All derived node types can be used interchangeably through base class pointers
- **Type System**: Derived types maintain the contract of their base types
- **Polymorphic Behavior**: Virtual functions ensure correct behavior regardless of actual object type
- **Collection Interfaces**: All collection types implement consistent interfaces
- **Value Types**: All value representations follow consistent contracts

#### Interface Segregation Principle (ISP)
- **Specialized Interfaces**: Separate interfaces for different concerns (e.g., `Visitable`, `Evaluable`)
- **Minimal Dependencies**: Classes only depend on interfaces they actually use
- **Role-Based Interfaces**: Different roles have different interface requirements
- **Parser Interfaces**: Separate interfaces for different parsing concerns
- **Registry Interfaces**: Focused interfaces for different registry operations

#### Dependency Inversion Principle (DIP)
- **Abstract Dependencies**: High-level modules depend on abstractions, not concretions
- **Dependency Injection**: Pass dependencies through constructors or parameters
- **Interface-Based Design**: Core components interact through well-defined interfaces
- **Service Abstractions**: File I/O, import management use abstract interfaces
- **Runtime Abstractions**: Memory management and garbage collection use abstract interfaces

### Core Design Principles

#### DRY (Don't Repeat Yourself)
- **Shared Utilities**: Common functionality consolidated in utility classes
- **Template Methods**: Base classes provide common algorithms with customizable steps
- **Code Generation**: AST node generation uses templates to avoid repetition
- **Shared Type Checking**: Common type validation logic centralized in TypeChecker
- **Error Handling**: Consistent error reporting mechanisms across all components
- **Test Frameworks**: Reusable test infrastructure for all test suites

#### KISS (Keep It Simple, Stupid)
- **Single-Purpose Classes**: Each class has one clear, well-defined responsibility
- **Simple Interfaces**: Minimal, focused interfaces with clear contracts
- **Straightforward Algorithms**: Prefer simple, readable implementations over clever optimizations
- **Clear Naming**: Self-documenting code with descriptive class and method names
- **Minimal Abstractions**: Only introduce abstractions when clearly beneficial
- **Direct Implementation**: Avoid over-engineering solutions

#### Kernighan's Law
*"Debugging is twice as hard as writing the code in the first place. Therefore, if you write the code as cleverly as possible, you are, by definition, not smart enough to debug it."*

- **Readable Code**: Prioritize code clarity over clever optimizations
- **Comprehensive Testing**: Extensive test suites to catch bugs early
- **Error Diagnostics**: Detailed error messages with source location information
- **Debugging Support**: Built-in debugging capabilities and verbose modes
- **Code Reviews**: Multi-person review process to catch complexity issues
- **Documentation**: Clear documentation of complex algorithms and design decisions

#### Zawinski's Law
*"Every program attempts to expand until it can read mail. Those programs which cannot so expand are replaced by ones which can."*

- **Feature Scope Control**: Clear boundaries on what mType should and shouldn't do
- **Core Focus**: Maintain focus on language interpretation and execution
- **Plugin Architecture**: Allow extensions without bloating the core
- **Modular Design**: Separate concerns to prevent feature creep
- **Well-Defined APIs**: Clear interfaces prevent uncontrolled expansion
- **Regular Refactoring**: Continuously evaluate and remove unnecessary features

### Advanced Design Approaches

#### Data-Driven Design
- **Configuration-Based Behavior**: Language features configured through data rather than code
- **AST Structure**: Tree structure drives execution flow
- **Type Definitions**: Type system behavior driven by type metadata
- **Test Data**: Test cases defined in data files rather than hardcoded
- **Grammar Definitions**: Parser behavior driven by grammar specifications
- **Error Messages**: Error reporting driven by message templates and data

**Implementation Examples:**
```cpp
// Type Registry - Data-driven type management
class TypeRegistry {
    std::unordered_map<std::string, TypeDefinition> types;
    std::vector<TypeConversionRule> conversionRules;

public:
    void registerType(const TypeDefinition& type);
    bool canConvert(const Type& from, const Type& to) const;
};

// Test Framework - Data-driven test execution
class TestSuite {
    std::vector<TestCase> testCases;

public:
    void loadTestsFromDirectory(const std::string& path);
    TestResults runTests(const TestFilter& filter);
};
```

#### Domain-Driven Design (DDD)
- **Language Domain**: Clear modeling of programming language concepts
- **Bounded Contexts**: Separate contexts for lexing, parsing, evaluation, and type checking
- **Domain Entities**: Core language constructs modeled as first-class entities
- **Value Objects**: Immutable objects for tokens, types, and values
- **Domain Services**: Services for complex domain operations
- **Repositories**: Data access patterns for symbol tables and type registries

**Domain Model Structure:**
```cpp
// Domain Entities
class ClassDefinition : public Entity {
    ClassId id;
    std::string name;
    std::vector<MethodDefinition> methods;
    std::vector<FieldDefinition> fields;
};

// Value Objects
class Type : public ValueObject {
    std::string name;
    std::vector<GenericParameter> genericParams;
    bool isNullable;
};

// Domain Services
class TypeChecker : public DomainService {
public:
    TypeCheckResult checkCompatibility(const Type& expected, const Type& actual);
    Type inferType(const Expression& expr, const Context& context);
};

// Repositories
class ClassRepository {
public:
    virtual ClassDefinition findById(const ClassId& id) = 0;
    virtual std::vector<ClassDefinition> findByNamespace(const std::string& ns) = 0;
    virtual void save(const ClassDefinition& classDef) = 0;
};
```

### Design Pattern Implementation

#### Visitor Pattern
- **AST Traversal**: Clean separation of operations from node structure
- **Type Checking**: Separate visitor for type validation
- **Code Generation**: Extensible code generation without modifying AST nodes
- **Optimization**: Multiple optimization passes as separate visitors

#### Strategy Pattern
- **Memory Management**: Different allocation strategies (ArrayPool, direct allocation)
- **Optimization Levels**: Different optimization strategies based on flags
- **Error Recovery**: Multiple error recovery strategies
- **Import Resolution**: Different import resolution strategies

#### Factory Pattern
- **AST Node Creation**: Centralized creation of AST nodes
- **Token Creation**: Consistent token creation in lexer
- **Value Creation**: Type-safe value object creation
- **Error Creation**: Consistent error object creation

#### Observer Pattern
- **Error Reporting**: Multiple error handlers can observe parsing/evaluation
- **Debug Events**: Debug listeners can observe execution events
- **Performance Monitoring**: Performance collectors observe operation timing

#### Command Pattern
- **Statement Execution**: Each statement type as executable command
- **Undo Operations**: Reversible operations for debugging
- **Batch Operations**: Grouped operations for optimization

### Implementation Mode Guidelines

#### Code Organization
- **Layered Architecture**: Clear separation between lexing, parsing, evaluation layers
- **Dependency Flow**: Dependencies flow inward toward domain core
- **Interface Boundaries**: Clear interfaces between major components
- **Module Cohesion**: High cohesion within modules, loose coupling between modules

#### Quality Assurance
- **Unit Testing**: Comprehensive unit tests for all components
- **Integration Testing**: End-to-end testing of language features
- **Performance Testing**: Benchmarks for critical operations
- **Memory Testing**: Leak detection and memory usage validation

#### Maintainability
- **Code Reviews**: Multi-person review for all changes
- **Documentation**: Comprehensive documentation of design decisions
- **Refactoring**: Regular refactoring to maintain code quality
- **Technical Debt Management**: Systematic approach to addressing technical debt

This implementation approach ensures that the mType language interpreter is not only functional but also maintainable, extensible, and robust in the face of changing requirements.

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

## Plan Mode and Development Roles

### Overview
The mType project utilizes a structured plan mode approach for development tasks, where different roles are assigned based on the type of work being performed. This ensures proper separation of concerns and expertise application.

### Development Roles

#### Language Designer
- **Responsibilities**:
  - Design new language features and syntax
  - Define grammar rules and language semantics
  - Plan language evolution and feature roadmaps
  - Ensure language consistency and usability
- **Focus Areas**:
  - Syntax design for new constructs
  - Type system enhancements
  - Language feature integration
  - Backward compatibility considerations

#### Compiler Engineer
- **Responsibilities**:
  - Implement lexer, parser, and AST components
  - Develop semantic analysis and type checking
  - Optimize compilation performance
  - Handle error reporting and recovery
- **Focus Areas**:
  - Lexical analysis improvements
  - Parser enhancement and error handling
  - AST node implementation
  - Semantic analyzer development

#### Runtime Developer
- **Responsibilities**:
  - Implement interpreter and execution engine
  - Develop memory management systems
  - Optimize runtime performance
  - Handle garbage collection and resource management
- **Focus Areas**:
  - Evaluator component development
  - Environment and scope management
  - Memory optimization (ArrayPool, StringPool)
  - Performance profiling and optimization

#### Type System Architect
- **Responsibilities**:
  - Design and implement type checking systems
  - Develop generic type support
  - Handle type inference and conversion
  - Ensure type safety across all language features
- **Focus Areas**:
  - Generic type implementation
  - Type conversion utilities
  - Interface type checking
  - Type registry management

#### Test Engineer
- **Responsibilities**:
  - Develop comprehensive test suites
  - Create test frameworks and utilities
  - Ensure code coverage and quality
  - Validate language feature implementations
- **Focus Areas**:
  - Test case development for all language features
  - Test framework enhancement
  - Integration testing
  - Performance testing and benchmarking

#### Tools Developer
- **Responsibilities**:
  - Develop IDE extensions and language support tools
  - Create debugging and development utilities
  - Implement language server features
  - Build developer experience tools
- **Focus Areas**:
  - VS Code extension development
  - Syntax highlighting and IntelliSense
  - Code formatting and analysis
  - Import management and navigation

#### Documentation Specialist
- **Responsibilities**:
  - Maintain comprehensive project documentation
  - Create user guides and API documentation
  - Ensure documentation accuracy and completeness
  - Plan documentation structure and organization
- **Focus Areas**:
  - Language reference documentation
  - Developer guides and tutorials
  - API documentation maintenance
  - Code example creation and validation

### Plan Mode Workflow

#### Feature Development Process
1. **Planning Phase**:
   - Language Designer defines feature requirements
   - Type System Architect evaluates type system impact
   - Compiler Engineer assesses implementation complexity
   - Test Engineer plans testing strategy

2. **Implementation Phase**:
   - Compiler Engineer implements core functionality
   - Runtime Developer handles execution aspects
   - Type System Architect ensures type safety
   - Tools Developer updates language support

3. **Validation Phase**:
   - Test Engineer validates implementation
   - Documentation Specialist updates documentation
   - All roles collaborate on integration testing
   - Performance evaluation and optimization

#### Role Collaboration Guidelines
- **Cross-functional Communication**: Regular coordination between roles
- **Code Review Process**: Multi-role review for critical components
- **Knowledge Sharing**: Documentation of design decisions and implementations
- **Quality Assurance**: Collaborative testing and validation processes

### Role Assignment Strategy

#### For New Language Features
- **Primary**: Language Designer + Type System Architect
- **Secondary**: Compiler Engineer + Runtime Developer
- **Support**: Test Engineer + Documentation Specialist

#### For Performance Optimization
- **Primary**: Runtime Developer + Compiler Engineer
- **Secondary**: Test Engineer (for benchmarking)
- **Support**: Documentation Specialist (for optimization guides)

#### For Tool Development
- **Primary**: Tools Developer
- **Secondary**: Language Designer (for language features)
- **Support**: Test Engineer + Documentation Specialist

#### For Bug Fixes and Maintenance
- **Primary**: Role responsible for affected component
- **Secondary**: Test Engineer (for regression testing)
- **Support**: Documentation Specialist (for updates)

This structured approach ensures that each aspect of the mType language development is handled by specialists with the appropriate expertise, leading to higher quality implementations and better overall project outcomes.

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

## Array Memory Management & Optimization

### FlatMultiArray Architecture
mType uses a sophisticated flat array architecture for multi-dimensional arrays that provides significant performance benefits:

- **Single Allocation**: Multi-dimensional arrays use one contiguous memory block instead of nested allocations
- **Stride-Based Indexing**: Efficient calculation of positions in flat buffer using stride arithmetic
- **Cache Optimization**: Contiguous memory layout improves CPU cache performance
- **Exception Safety**: Single allocation eliminates partial construction memory leaks

### ArrayPool System
The ArrayPool provides intelligent memory reuse for common array patterns:

#### Pooling Strategy
- **Common Patterns**: Powers of 2, small matrices (2x2, 3x3, 4x4, 8x8, 16x16)
- **Size Limits**: Arrays up to 1M elements with dimensions ≤ 1024
- **Automatic Management**: Pool sizing and cleanup handled automatically
- **Thread Safety**: Mutex-protected operations for concurrent access

#### Performance Benefits
- **Reduced Allocations**: Reuses arrays for common dimension patterns
- **Memory Efficiency**: Eliminates frequent allocation/deallocation overhead
- **Hit Rate Tracking**: Statistics available for optimization analysis
- **Fallback Handling**: Large or uncommon arrays bypass pool gracefully

#### Pool Configuration
```cpp
// Pool automatically handles these patterns efficiently:
int[][] matrix2x2 = new int[2][2];     // Pooled - common pattern
int[][] matrix4x4 = new int[4][4];     // Pooled - power of 2
int[][] large = new int[200][200];     // Not pooled - too large
```

### Array Creation Flow
1. **Dimension Evaluation**: Parse and validate array size expressions
2. **Pool Check**: Determine if dimensions match poolable patterns
3. **Allocation Strategy**:
   - **1D Arrays**: Use NativeArray for optimal single-dimension performance
   - **2D+ Poolable**: Acquire from ArrayPool with automatic reset
   - **2D+ Large**: Direct FlatMultiArray allocation
4. **Initialization**: Fill with appropriate default values based on element type

### Exception Safety Improvements
- **RAII Design**: Automatic cleanup via smart pointers and RAII guards
- **Pool Validation**: Dimension verification prevents pool corruption
- **Reset Mechanism**: Pooled arrays properly reset between uses
- **Memory Leak Prevention**: Eliminated recursive allocation vulnerabilities

### Recent Fixes (2024)
✅ **ArrayPool Reuse Bug**: Fixed pool returning new arrays instead of reusing pooled ones
✅ **Memory Leak Elimination**: Removed legacy recursive allocation method
✅ **Exception Safety**: Added comprehensive null checks and validation
✅ **Performance Validation**: All 229+ tests pass with improved efficiency

## Contributing Guidelines

- Follow the established coding style consistently
- Write comprehensive tests for new features
- Maintain backward compatibility where possible
- Document public APIs thoroughly
- Consider performance implications of changes
- Use modern C++ features appropriately
- Test collection operations with various object types
- Verify content-based hashing works correctly for nested structures