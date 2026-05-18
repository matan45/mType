# mType Domain Context

Domain vocabulary used by the `/improve-codebase-architecture` skill and other planning workflows. Keep entries short and load-bearing — this file is for shared language, not user docs.

## Parsing

- **Parser** — deep module that turns a token stream into an AST. Single class `parser::Parser`, implementation split across `Parser_*.cpp` partials (Core/Class/Statement/Expression) for compile-time parallelism. External callers see only `parseProgram()` and `getImportManager()`. Internal layering is by grammar region, not file boundaries.
- **Token** — lexer output unit (`token::Token`). Consumed via `parser::TokenStream`, the cursor abstraction with `current()`, `advance()`, `peekAhead()`, `expect()`.
- **AST** — the `ast::ASTNode` hierarchy. Parser produces; optimizer rewrites; BytecodeCompiler consumes. Nodes are grouped under `ast/nodes/{classes,expressions,functions,statements}/`.
- **GenericType** — recursive type descriptor (`ast::GenericType`). Built by `parser::TypeParser` directly from the token stream (no callbacks into Parser).
- **ParserContextState** — parser-private state (`insideClass`, `insideAsync`, `insideFunction`, `insideInterface`, `insideConstructor`, `recursionDepth`) with RAII guards. Lives as a member of `Parser`.
- **TypeParser** — free-standing parser for `GenericType` syntax. Kept separate from `Parser` because it has multiple callers (ParameterParser, MethodParser, FieldParser) and no recursive callback to expression parsing.
- **StatementTypeDetector** — lookahead dispatch table mapping a token window to a `StatementType` enum. Used by `Parser` to choose which body to invoke per statement.
- **TypeRegistry (parser-local)** — tracks declared class/interface/function names *during parsing* for duplicate-declaration diagnostics. Distinct from the runtime `types::TypeRegistry`.

## Compilation pipeline

- **Bytecode** — `vm::bytecode::BytecodeProgram` is the post-compile artifact (`.mtc` files). Holds an `Instruction` stream + constant pool + class metadata.
- **BytecodeCompiler** — walks AST and emits `Instruction`s. Currently fragmented across `vm/compiler/visitors/` and `vm/compiler/registration/` (deepening candidate).
- **OptimizationService** — owns AST optimization passes (constant folding, dead-code elimination, unused-declaration elimination). Runs between Parser and BytecodeCompiler when `-release` is set.

## Runtime

- **VirtualMachine** — stack-based execution engine. Holds the call stack and dispatches instructions to category-specific **Executors**.
- **Executor** — handler for one category of opcodes (`ArithmeticExecutor`, `ControlFlowExecutor`, `FunctionExecutor`, …). Receive an `ExecutionContext` reference.
- **ExecutionContext** — current call frame's mutable state plus references to program/stack/loaded-programs. Currently a god-helper threaded into every executor (deepening candidate).
- **Environment** — owns runtime registries: `ClassRegistry`, `FunctionRegistry`, `VariableManager`, `ScopeManager`, `NativeRegistry`. The seam between scoping (`manager/`) and symbol tables (`registry/`) is currently muddy.
- **Value** — tagged union (`value::Value`) carrying a `ValueType` tag and a payload. Primitives, references to heap objects, arrays, lambdas, strings (interned via `StringPool`).

## Types

- A type in mType currently has four overlapping representations: AST `GenericType`, `types::TypeRegistry` entry, `runtimeTypes::Definition`, and `runtimeTypes::klass::ClassDefinition`. Unification is a known deepening candidate; until then, each layer has a specific role:
  - **GenericType (AST)** — parse-time descriptor with raw type strings.
  - **TypeRegistry (types/)** — catalog of known types and conversion rules.
  - **Definition (runtimeTypes/)** — runtime metadata base class (methods, fields).
  - **ClassDefinition** — `Definition` specialized for user-defined classes.

## Plugins & native code

- **Plugin** — runtime-loaded shared library exposing the C ABI in `plugin/PluginHostApi.h`. Routed through `NativeDelegate`; unloading invalidates `BytecodeProgram::clearNativeCacheSlots()`.

## Architectural exceptions (line-count rule)

These files are deliberately allowed to exceed the 500-line `.cpp` cap because they are **deep modules** — large behaviour behind a small interface:

- `parser/Parser.hpp` (the class declaration, post-consolidation)
- `parser/TypeParser.cpp`
- `parser/utilities/StatementTypeDetector.cpp`

Each `Parser_*.cpp` partial still targets the 500-line guideline.
