# mType Language Project

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
│   ├── optimizer/               # AST optimization passes
│   │   ├── OptimizationService.cpp/.hpp
│   │   └── passes/              # Individual optimization passes
│   │       ├── ConstantFoldingPass.cpp/.hpp
│   │       ├── DeadCodeEliminationPass.cpp/.hpp
│   │       └── UnusedDeclarationEliminationPass.cpp/.hpp
│   ├── vm/                      # Virtual Machine components
│   │   ├── bytecode/            # Bytecode definitions and serialization
│   │   │   ├── BytecodeProgram.cpp/.hpp
│   │   │   ├── Instruction.hpp
│   │   │   └── OpCode.hpp
│   │   ├── compiler/            # Bytecode compiler
│   │   │   ├── BytecodeCompiler.cpp/.hpp
│   │   │   └── registration/    # Class/method registration
│   │   └── runtime/             # VM execution engine
│   │       ├── VirtualMachine.cpp/.hpp
│   │       ├── context/         # Execution context
│   │       ├── executors/       # Instruction executors
│   │       ├── stack/           # Stack management
│   │       └── utils/           # VM utilities
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

## Architecture Update (v0.2.0)

**Migration from Tree-Walking Interpreter to Bytecode VM**

mType has undergone a significant architectural transformation, moving from a tree-walking interpreter to a **bytecode compilation + VM execution** model. This change provides:
- **Better Performance**: Stack-based VM execution is faster than recursive tree-walking
- **Optimizations**: AST optimizations before bytecode generation (constant folding, dead code elimination)
- **Portability**: Compiled bytecode can be distributed and executed independently
- **Future Capabilities**: Foundation for JIT compilation and advanced optimizations

### Compilation Pipeline

```
Source Code (.mt)
    ↓
Lexer (Tokenization)
    ↓
Parser (AST Construction)
    ↓
AST Optimizer (Optional - based on optimization level)
    ├─ Constant Folding Pass
    ├─ Dead Code Elimination Pass
    └─ Unused Declaration Elimination Pass
    ↓
Bytecode Compiler
    ↓
Bytecode Program (.mtc)
    ↓
Virtual Machine (Stack-based execution)
    ↓
Result
```

### Execution Modes

1. **Direct Execution**: `mType script.mt`
   - Compiles AST to bytecode in memory
   - Executes immediately via VM
   - No intermediate `.mtc` file created

2. **Compile to Bytecode**: `mType --compile [-release] script.mt`
   - Generates `script.mtc` bytecode file
   - `-release` flag enables full optimizations
   - Bytecode includes class metadata, constant pool, debug information

3. **Execute Bytecode**: `mType script.mtc`
   - Loads and deserializes bytecode
   - Registers class metadata
   - Executes via VM

### Key Components

#### Bytecode Compiler (`vm/compiler/BytecodeCompiler`)
- Translates AST to stack-based bytecode instructions
- Generates constant pools for strings, numbers, etc.
- Emits class metadata for runtime registration
- Preserves source locations for debugging

#### Virtual Machine (`vm/runtime/VirtualMachine`)
- Stack-based execution engine
- Specialized instruction executors (SOLID principle)
- Call stack management with overflow detection
- Debugger integration support
- Async/await support via event loop

#### Optimization Service (`optimizer/OptimizationService`)
- Configurable optimization levels (Debug, Release)
- AST-level optimizations before bytecode generation
- Preserves annotations through optimization passes
- Dead code elimination respects `@Script` and `@Throw` annotations

### Removed Components (v0.2.0)

The following components were removed during the bytecode migration:

- **Tree-Walking Interpreter** (`evaluator/` directory)
  - `Evaluator.cpp/.hpp`
  - `EvaluatorCoordinator.cpp/.hpp`
  - `ExpressionEvaluator.cpp/.hpp`
  - `ObjectEvaluator.cpp/.hpp`
  - `StatementEvaluator.cpp/.hpp`
  - All evaluation utilities and helpers

- **Dual Execution Mode Support**
  - `ASTExecutionStrategy.cpp/.hpp`
  - `DualValidationStrategy.cpp/.hpp`
  - Execution mode selection logic

- **Direct AST Evaluation**
  - AST is now only used for compilation, not interpretation
  - No runtime AST traversal

### VM Configuration

The Virtual Machine supports configurable parameters:

```cpp
// Default configuration (constants::vm namespace)
constexpr size_t DEFAULT_CALL_STACK_CAPACITY = 64;    // Initial reservation
constexpr size_t DEFAULT_MAX_CALL_STACK_SIZE = 1000;  // Maximum depth

// Custom configuration via constructor
VirtualMachine vm(environment, maxStackDepth);
```

**Stack Overflow Protection**: The VM includes overflow detection with helpful error messages showing the call stack trace to identify infinite recursion patterns.

## Implementation Mode and Design Principles

### SOLID Principles Implementation

#### Single Responsibility Principle (SRP)
- **Lexer**: Solely responsible for tokenizing input
- **Parser**: Only handles syntax analysis and AST construction
- **BytecodeCompiler**: Focused exclusively on translating AST to bytecode
- **VirtualMachine**: Dedicated to bytecode execution
- **Instruction Executors**: Each executor handles one category of instructions
- **OptimizationService**: Manages AST optimization passes
- **ErrorReporter**: Handles all error reporting and formatting
- **Environment**: Manages variable scoping and symbol tables
- **TypeRegistry**: Maintains type definitions and relationships

#### Open/Closed Principle (OCP)
- **ASTNode Hierarchy**: Base `ASTNode` class with virtual methods, extensible through inheritance
- **Visitor Pattern**: Used for AST traversal, allowing new operations without modifying existing nodes
- **Strategy Pattern**: Different optimization strategies can be plugged in without changing core logic
- **Plugin Architecture**: New language features can be added without modifying core components
- **Instruction Executors**: New instruction types can be added by implementing new executors
- **Optimization Passes**: New optimization passes can be added without modifying existing ones

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

#### Class Design Best Practices
- **File Structure**: Each class in its own `.hpp/.cpp` file pair
- **Function Size Limit**: Maximum 50 lines per function implementation
- **Class Size Limit**: Maximum 500 lines in `.cpp` implementation files
- **Helper Classes**: Extract complex logic into focused helper classes
- **Utility Extraction**: Move common operations to utility classes
- **Context Grouping**: Organize classes by functional context in folder structure


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