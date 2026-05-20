# Refactor Reference

Detailed mechanics for the four operation types listed in `SKILL.md`. Each section assumes the plan-first checklist is already complete.

## 1. Split oversized file

Goal: bring a `.cpp` / `.hpp` / `.ts` file under 500 lines without changing the public interface.

### When the file is a top-level parser / compiler
The deep-module exception in `CLAUDE.md` exists *because* prior splits made the code worse — `ParseContext`-style indirection was reversed in favor of one cohesive grammar region per file. Before splitting `parser/ClassParser.cpp`, `parser/StatementParser.cpp`, `parser/ExpressionParser.cpp`, `parser/InterfaceParser.cpp`, `parser/TypeParser.cpp`, or `parser/utilities/StatementTypeDetector.cpp`, re-read that history. If the new split mirrors the abandoned design, stop.

### Mechanics (for files outside that list)

1. Identify a **cohesive subset** of methods — same private data, same lifecycle, no cross-talk with the remainder. If everything talks to everything, splitting only moves the mess.
2. Extract into a sibling file in the same folder. For C++:
   - Name the new file by responsibility (`ExpressionCompiler_Binary.cpp` → wrong; `BinaryExpressionEmitter.cpp` → right).
   - Either (a) move the methods into a new class composed by the original, or (b) keep them as free functions in an anonymous namespace if they were already file-local helpers.
   - Add the new file to `premake5.lua` filters and `mType.vcxproj.filters`.
3. For TypeScript (`mtype-vscode-extension/src/`):
   - Match folder layout to feature (`completion/`, `analyzer/`, etc.).
   - Update barrel exports if the area uses them; otherwise update direct imports.
4. Verify the public-facing type still has the exact same interface (signatures, includes it exposes, exception types).
5. Build (user runs build), then run the area's tests.

### Splitting a `.hpp`

Header splits ripple through every translation unit that includes the file — proceed with eyes open.

When the header is over 500 lines, first check whether it's *content-heavy* (a large enum like `OpCode.hpp`, a value-type definition, a SIMD policy table) or *interface-heavy* (many unrelated public methods on one type). Content-heavy headers usually deserve a documented exception in `CLAUDE.md`. Interface-heavy headers are real refactor candidates:

- Split by **responsibility cluster**, not by line count. If half the methods touch one piece of state and the other half touch another, that's two types pretending to be one.
- Forward-declare aggressively in the new headers to limit include fan-out.
- If the header exposes large inline templates, moving them to a `*_inline.hpp` that the public header includes at the bottom keeps the public surface readable without breaking inlining.
- After splitting, `Grep` every `#include` of the old header and confirm consumers compile — header-split fallout shows up at the compiler stage, not at runtime.

## 2. Extract / inline / rename

### Extract method

- Hard limit trigger: function > 50 lines, or a block with a clearly nameable intent.
- Extract within the same class unless the helper is genuinely reused — premature standalone utilities violate the deletion test.
- Update the call site; keep the extracted method `private`.

### Extract class / module

- The new class earns its keep only if it has **own state** or **own lifecycle**. A stateless bag of static methods is a namespace, not a class.
- For C++: place new class in `mType/<context>/`, matching neighboring code's folder grouping (`ast/nodes/expressions/`, `vm/runtime/executors/`, etc.).
- For TS: align with `mtype-vscode-extension/src/<feature>/`.

### Inline

- Inverse of extract. Apply when a helper has exactly one caller and the abstraction earns nothing.
- Confirm zero call sites elsewhere via `Grep` before inlining.

### Rename

- Use `Grep` to enumerate every occurrence, then `Edit`/`Edit replace_all` per-file. Do **not** use a blind project-wide rename — bytecode constant pool string literals, registration tables (`vm/compiler/registration/`), and test file names may hold the same identifier with different meaning.
- Renaming a `runtimeTypes/klass/` class name affects:
  - The bytecode constant pool layout — old `.mtc` files become unreadable. Confirm with the user before renaming registered types.
  - VS Code extension symbol tables (`mtype-vscode-extension/src/analyzer/`).
- Renaming an opcode (`vm/bytecode/OpCode.hpp`) breaks serialized bytecode compatibility. Treat as a versioned change, not a refactor.

## 3. Restructure module layout

### When to move files

- The file's neighbors no longer match its responsibility (e.g. an `expressions/` node that grew into a statement).
- A new context folder would absorb several scattered files and reduce search distance.
- An ADR or `CLAUDE.md` revision mandates the new layout.

### Mechanics

1. List every file to move + its new location. No file moves without its companion (`.hpp` + `.cpp` move together; TS source + its tests move together).
2. Update `#include` paths in every consumer — `Grep` for the old path.
3. Update build config for the affected subproject:
   - `mType/`: root `premake5.lua` + the relevant `mtype-*.vcxproj.filters` (each module has its own — `mtype-ast`, `mtype-jit`, `mtype-frontend`, etc.).
   - `languageserver/`: `languageserver/premake5.lua` + `mtype-language-server.vcxproj.filters` (or `-lib.vcxproj.filters` for shared code, `-tests.vcxproj.filters` for tests).
   - `packagemanager/`: `packagemanager/premake5.lua` + `mtpm.vcxproj.filters`.
   - `mtype-vscode-extension/tsconfig.json` `paths` aliases if used.
4. Update any documentation referencing the old path (`CLAUDE.md`'s tree, READMEs).
5. Build, then run tests.

### Cross-module moves (e.g. between `parser/` and `ast/`)

These are not refactors — they are redesigns. Confirm with the user before starting; the layered architecture in `CLAUDE.md` is load-bearing.

### Cross-subproject moves

Moving code between `mType/`, `languageserver/`, `packagemanager/`, and `mtype-vscode-extension/` is almost never a refactor:

- `mType/` ⇄ `languageserver/` — the LSP intentionally consumes mType as a library. Shared code lives in `mType/` and the LSP includes it; the reverse direction is a design break.
- `packagemanager/` is standalone (its own `.sln`); pulling its types into `mType/` couples the runtime to the package manager.
- `mtype-vscode-extension/` ⇄ everything — different language (TS), different process. The boundary is the LSP wire protocol, not source-level sharing.

Confirm with the user before any cross-subproject move.

## 4. Pattern-driven (SOLID)

### Visitor

Applies when:
- A new operation must be added across the AST hierarchy without modifying each node.
- The existing `ASTVisitor` already covers similar operations — extend, don't duplicate.

Don't apply when:
- There is exactly one operation. A virtual method on `ASTNode` is simpler.

### Strategy

Applies when:
- Multiple interchangeable algorithms exist (optimization passes in `optimizer/passes/`, memory-allocation strategies in `value/`).
- The choice between them is data-driven (config flag, optimization level).

Don't apply when:
- There is exactly one strategy and "future variants" are speculative.

### Factory

Applies when:
- Construction logic is non-trivial (AST node creation, value construction with type negotiation).
- Multiple call sites would otherwise duplicate construction logic.

### Observer

Applies when:
- Multiple independent listeners need notification (debug hooks, error reporters).
- Listeners must be added/removed at runtime.

### Justification template

Before introducing any pattern, fill this in:

- **Concrete callers** — list ≥ 2 real call sites that benefit. If only 1 exists, it's premature.
- **Deletion test** — if I remove this abstraction, where does the complexity reappear? If "spread across N callers in a way each one would have to handle," the pattern earns its keep.
- **Locality cost** — the abstraction adds one indirection. Is the leverage worth the bounce?

If any answer is weak, drop the pattern and write the simpler direct code.

## Cross-cutting reminders

- Comment cleanup: remove `MYT-*` ticket markers from touched comments — keep the technical content, drop the ticket reference. Delete commented-out code.
- Dead code: confirm via `Grep`, compiler diagnostics, tests, and surrounding architecture. Plugin-facing / reflection-facing / test-discovered code is "alive" even if it looks orphaned.
- Build invocation: the user runs builds manually (see `memory/feedback_user_runs_build.md`). Do not invoke `premake5` or `msbuild` from this skill.
