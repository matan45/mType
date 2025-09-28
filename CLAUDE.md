# mType Language Project

## Overview
mType is a modern programming language implementation with a focus on type safety, object-oriented programming, and performance optimization. The project includes a complete interpreter with lexical analysis, parsing, semantic analysis, and execution capabilities.

## Features

### Core Language Features
- **Static Type System**: Strong type checking with compile-time error detection
- **Object-Oriented Programming**: Full support for classes, inheritance, encapsulation
- **Native Function Support**: Integration with C++ native functions and libraries
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
- **Circular Dependency Detection**: Prevents infinite import loops
- **Module Resolution**: Automatic resolution of import paths
- **Dependency Management**: Handles complex import hierarchies

### Native Function Integration
- **C++ Function Registration**: Register C++ functions to be callable from mType
  ```cpp
  interpreter->registerNativeFunction("createPoint", [](const std::vector<Value>& args) -> Value {
      // Extract arguments and create Point object
      float x = std::get<float>(args[0]);
      float y = std::get<float>(args[1]);

      // Create and return Point object instance
      auto pointInstance = std::make_shared<ObjectInstance>(pointClass);
      pointInstance->setField("x", x);
      pointInstance->setField("y", y);
      return pointInstance;
  });
  ```
- **Object Creation**: Native functions can create and return mType objects
- **Type Conversion**: Automatic conversion between C++ and mType types
- **Library Integration**: Seamless integration with existing C++ libraries
- **Performance**: Direct C++ execution for performance-critical operations

**Native Function Usage:**
```mtype
// Call native function from mType code
Point pt = createPoint(4.0, 5.0);  // Native C++ creates Point object
float distance = pt.distanceFromOrigin();  // Call mType method on native-created object
```

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
- `tests/testFiles/native/` - Native function integration testing
- `tests/testFiles/integration/` - Complex integration scenarios
- `tests/testFiles/error/` - Error handling and edge cases

### Native Function Testing
The `NativeTest` class demonstrates comprehensive testing of native function integration:

**Test Coverage:**
- **Native Function Registration**: Testing `registerNativeFunction()` API
- **Object Creation**: Native functions creating mType objects
- **Static Method Calls**: C++ API calling mType static methods
- **Type Conversion**: Automatic conversion between C++ and mType types
- **Memory Management**: Proper object lifetime and memory safety

**Example Test (from `NativeTest.cpp`):**
```cpp
// Register native createPoint function
interpreter->registerNativeFunction("createPoint", [this](const std::vector<Value>& args) -> Value {
    // Extract and convert arguments
    float x = std::holds_alternative<int>(args[0]) ?
              static_cast<float>(std::get<int>(args[0])) :
              std::get<float>(args[0]);
    float y = std::holds_alternative<int>(args[1]) ?
              static_cast<float>(std::get<int>(args[1])) :
              std::get<float>(args[1]);

    // Create Point object from C++
    auto pointClass = classRegistry->findClass("Point");
    auto pointInstance = std::make_shared<ObjectInstance>(pointClass);
    pointInstance->setField("x", x);
    pointInstance->setField("y", y);

    return pointInstance;
});

// Test usage from mType
Point pt = createPoint(4, 5);  // Native C++ creates Point object
```

**Native Test Example (`nativeExample.mt`):**
```mtype
class Point {
    float x;
    float y;

    constructor(float px, float py) {
        x = px;
        y = py;
    }

    function distanceFromOrigin(): float {
        return sqrt(x * x + y * y);
    }

    static function createOrigin(): Point {
        return new Point(0.0, 0.0);
    }
}

class MathUtils {
    static final float PI = 3.14159;
    static int operationCount = 0;

    static function square(int x): int {
        operationCount = operationCount + 1;
        return x * x;
    }

    static function circleArea(float radius): float {
        operationCount = operationCount + 1;
        return PI * radius * radius;
    }
}

// Native function usage
Point pt = createPoint(4,5);  // Native C++ creates Point object
```

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

## Advanced C++ Implementation Techniques

### Smart Pointer Usage
```cpp
// Use unique_ptr for exclusive ownership
std::unique_ptr<ASTNode> parseExpression();

// Use shared_ptr for shared ownership (AST nodes referenced by multiple parents)
std::shared_ptr<ClassDefinition> classRef;

// Use weak_ptr to break circular references
std::weak_ptr<Environment> parentEnv;

// Custom deleters for special cleanup
std::unique_ptr<Resource, CustomDeleter> resource;
```

### RAII Patterns
```cpp
// Scope guards for automatic cleanup
class ScopeGuard {
    std::function<void()> cleanup;
public:
    ScopeGuard(std::function<void()> f) : cleanup(f) {}
    ~ScopeGuard() { cleanup(); }
};

// Memory pool for frequent allocations
class ObjectPool {
    std::vector<std::unique_ptr<Object>> objects;
    std::stack<Object*> available;
public:
    Object* acquire();
    void release(Object* obj);
};
```

### Move Semantics and Perfect Forwarding
```cpp
// Move constructors for expensive operations
class AST {
    std::vector<std::unique_ptr<ASTNode>> nodes;
public:
    AST(AST&& other) noexcept : nodes(std::move(other.nodes)) {}
    AST& operator=(AST&& other) noexcept {
        nodes = std::move(other.nodes);
        return *this;
    }
};

// Perfect forwarding for factory methods
template<typename T, typename... Args>
std::unique_ptr<T> makeNode(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
```

### Exception Safety
```cpp
// Strong exception safety guarantee
class Environment {
    std::unordered_map<std::string, Value> variables;
public:
    void addVariable(const std::string& name, Value value) {
        auto temp = variables; // Copy current state
        temp[name] = std::move(value); // Modify copy
        variables = std::move(temp); // Commit if no exception
    }
};

// RAII for transaction-like operations
class EnvironmentTransaction {
    Environment& env;
    std::unordered_map<std::string, Value> backup;
    bool committed = false;
public:
    EnvironmentTransaction(Environment& e) : env(e), backup(e.getVariables()) {}
    ~EnvironmentTransaction() { if (!committed) env.restore(backup); }
    void commit() { committed = true; }
};
```

### Custom Exception Hierarchy
```cpp
// Base exception with source location
class InterpreterException : public std::exception {
protected:
    std::string message;
    SourceLocation location;
public:
    InterpreterException(std::string msg, SourceLocation loc)
        : message(std::move(msg)), location(loc) {}

    const char* what() const noexcept override { return message.c_str(); }
    const SourceLocation& getLocation() const { return location; }
};

// Specific exception types
class TypeMismatchException : public InterpreterException {
public:
    TypeMismatchException(const Type& expected, const Type& actual, SourceLocation loc)
        : InterpreterException(formatMessage(expected, actual), loc) {}
};

class RuntimeException : public InterpreterException {
public:
    RuntimeException(std::string msg, SourceLocation loc)
        : InterpreterException(std::move(msg), loc) {}
};
```

### Visitor Pattern with std::variant
```cpp
// Value type system using std::variant for runtime values
using Value = std::variant<int, float, bool, std::string, InternedString, std::monostate,
                          std::shared_ptr<ObjectInstance>,
                          std::shared_ptr<NativeArray>,
                          std::shared_ptr<FlatMultiArray>,
                          std::shared_ptr<SparseMultiArray>,
                          std::shared_ptr<LambdaValue>,
                          nullptr_t>;

// Helper function using std::visit with if constexpr
inline ValueType getValueType(const Value& value) {
    // Explicit checks for complex types before std::visit
    if (std::holds_alternative<std::shared_ptr<FlatMultiArray>>(value)) {
        return ValueType::ARRAY;
    }
    if (std::holds_alternative<std::shared_ptr<SparseMultiArray>>(value)) {
        return ValueType::ARRAY;
    }
    if (std::holds_alternative<std::shared_ptr<LambdaValue>>(value)) {
        return ValueType::LAMBDA;
    }

    // Generic visitor with compile-time type dispatch
    return std::visit([](const auto& v) -> ValueType {
        if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
            return ValueType::INT;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, float>) {
            return ValueType::FLOAT;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
            return ValueType::BOOL;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
            return ValueType::STRING;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, InternedString>) {
            return ValueType::STRING;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) {
            return ValueType::VOID;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<ObjectInstance>>) {
            return ValueType::OBJECT;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::shared_ptr<NativeArray>>) {
            return ValueType::ARRAY;
        } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, nullptr_t>) {
            return ValueType::NULL_TYPE;
        } else {
            return ValueType::VOID;
        }
    }, value);
}

// Generic type system using std::variant for type representation
class GenericType {
private:
    // Either a concrete type or a generic type parameter name
    std::variant<value::ValueType, std::string> baseType;
    // Type arguments for parameterized types (e.g., Array<T> has [T])
    std::vector<std::shared_ptr<GenericType>> typeArguments;

public:
    // Constructor for concrete types
    explicit GenericType(value::ValueType type) : baseType(type) {}

    // Constructor for generic type parameters
    explicit GenericType(const std::string& genericName) : baseType(genericName) {}

    // Check if this represents a generic type parameter using std::holds_alternative
    bool isGenericParameter() const {
        return std::holds_alternative<std::string>(baseType);
    }

    // Get base type name using std::visit
    std::string getBaseTypeName() const {
        return std::visit([](const auto& type) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(type)>, value::ValueType>) {
                return valueTypeToString(type);
            } else {
                return type; // string case
            }
        }, baseType);
    }
};

// Example of processing values with visitor pattern
template<typename Processor>
auto processValue(const Value& value, Processor&& processor) {
    return std::visit([&processor](const auto& v) {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, int>) {
            return processor.processInt(v);
        } else if constexpr (std::is_same_v<T, float>) {
            return processor.processFloat(v);
        } else if constexpr (std::is_same_v<T, bool>) {
            return processor.processBool(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return processor.processString(v);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<ObjectInstance>>) {
            return processor.processObject(v);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<NativeArray>>) {
            return processor.processArray(v);
        } else {
            return processor.processDefault();
        }
    }, value);
}
```

### CPU Cache Optimization
```cpp
// Data structure layout for cache efficiency
struct CacheOptimizedToken {
    TokenType type;        // 4 bytes
    uint32_t line;        // 4 bytes
    uint32_t column;      // 4 bytes
    uint32_t length;      // 4 bytes
    // Total: 16 bytes (cache line friendly)

    const char* text;     // Separate pointer to avoid cache pollution
};

// Memory pool with cache-aligned allocation
class AlignedPool {
    static constexpr size_t CACHE_LINE_SIZE = 64;
    std::vector<alignas(CACHE_LINE_SIZE) std::byte> memory;
    size_t offset = 0;
public:
    template<typename T>
    T* allocate(size_t count = 1) {
        size_t size = sizeof(T) * count;
        size_t aligned_offset = (offset + alignof(T) - 1) & ~(alignof(T) - 1);

        if (aligned_offset + size > memory.size()) {
            throw std::bad_alloc();
        }

        T* ptr = reinterpret_cast<T*>(memory.data() + aligned_offset);
        offset = aligned_offset + size;
        return ptr;
    }
};
```

### Self-Documenting Code
```cpp
// Use strong types instead of primitives
class LineNumber {
    int value;
public:
    explicit LineNumber(int v) : value(v) {}
    int get() const { return value; }

    LineNumber& operator++() { ++value; return *this; }
    bool operator<(const LineNumber& other) const { return value < other.value; }
};

class ColumnNumber {
    int value;
public:
    explicit ColumnNumber(int v) : value(v) {}
    int get() const { return value; }

    ColumnNumber& operator++() { ++value; return *this; }
    bool operator<(const ColumnNumber& other) const { return value < other.value; }
};

// Function signatures become self-documenting
SourceLocation createLocation(LineNumber line, ColumnNumber column);

// Named parameter idiom for complex constructors
class TokenBuilder {
    TokenType type_;
    std::string text_;
    LineNumber line_{1};
    ColumnNumber column_{1};
public:
    TokenBuilder& type(TokenType t) { type_ = t; return *this; }
    TokenBuilder& text(std::string t) { text_ = std::move(t); return *this; }
    TokenBuilder& line(LineNumber l) { line_ = l; return *this; }
    TokenBuilder& column(ColumnNumber c) { column_ = c; return *this; }

    Token build() const { return Token{type_, text_, line_, column_}; }
};
```

### Static Analysis Integration
```cpp
// Annotations for static analysis tools
[[nodiscard]] ParseResult parseExpression();
[[maybe_unused]] static const char* getVersionString();

// GSL (Guidelines Support Library) annotations
#include <gsl/gsl>

class Parser {
public:
    // Indicates parameter must not be null
    ParseResult parse(gsl::not_null<const char*> input);

    // Indicates returned pointer ownership
    gsl::owner<ASTNode*> createNode();

    // Span for safe array access
    void processTokens(gsl::span<const Token> tokens);
};

// Static assertions for compile-time checks
template<typename T>
class TypedValue {
    static_assert(std::is_trivially_copyable_v<T>,
                 "TypedValue requires trivially copyable types");
    static_assert(!std::is_pointer_v<T>,
                 "TypedValue should not store raw pointers");
    T value;
public:
    explicit TypedValue(T v) : value(std::move(v)) {}
    const T& get() const noexcept { return value; }
};

// Contract programming style with assertions
class Environment {
public:
    void setValue(const std::string& name, Value value) {
        // Precondition
        assert(!name.empty() && "Variable name cannot be empty");

        variables[name] = std::move(value);

        // Postcondition
        assert(variables.count(name) == 1 && "Variable must be stored");
    }
};
```

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
"bin\mType\Debug\x64\mType.exe" --test control       # Control Flow Test Suite
"bin\mType\Debug\x64\mType.exe" --test import        # Import Test Suite
"bin\mType\Debug\x64\mType.exe" --test class         # Class Test Suite
"bin\mType\Debug\x64\mType.exe" --test interface     # Interface Test Suite
"bin\mType\Debug\x64\mType.exe" --test lambda        # Lambda Test Suite
"bin\mType\Debug\x64\mType.exe" --test error         # Error Test Suite
"bin\mType\Debug\x64\mType.exe" --test integration   # Integration Test Suite
"bin\mType\Debug\x64\mType.exe" --test type          # Type Checking Test Suite
"bin\mType\Debug\x64\mType.exe" --test generics      # Generics Test Suite
"bin\mType\Debug\x64\mType.exe" --test arrays        # Array Test Suite
"bin\mType\Debug\x64\mType.exe" --test stringpool    # String Pool Test Suite
"bin\mType\Debug\x64\mType.exe" --test native        # Native C++ Integration Test Suite

# Alternative names for some test suites
"bin\mType\Debug\x64\mType.exe" --test controlflow   # Same as control
"bin\mType\Debug\x64\mType.exe" --test classes       # Same as class
"bin\mType\Debug\x64\mType.exe" --test interfaces    # Same as interface
"bin\mType\Debug\x64\mType.exe" --test lambdas       # Same as lambda
"bin\mType\Debug\x64\mType.exe" --test errors        # Same as error
"bin\mType\Debug\x64\mType.exe" --test typechecking  # Same as type
"bin\mType\Debug\x64\mType.exe" --test generic       # Same as generics
"bin\mType\Debug\x64\mType.exe" --test array         # Same as arrays
"bin\mType\Debug\x64\mType.exe" --test strings       # Same as stringpool

# Get help and see available test suites
"bin\mType\Debug\x64\mType.exe" --help
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