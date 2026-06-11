# mType Language Feature Tests — Reference

Detailed mechanics. See [SKILL.md](SKILL.md) for the workflow.

## Suite map

Which suite owns which language feature. Test fixtures live under `mType/tests/testFiles/<dir>/{pass,error}/`. Suite class is in `mType/tests/suites/<Name>TestSuite.{hpp,cpp}` and registered in `mType/run/TestUtilities.cpp`.

| Feature area                          | Suite                          | testFiles dir         |
|---------------------------------------|--------------------------------|-----------------------|
| Casts, `(T)x`, `isClassOf`, instanceOf | `CastTestSuite`                | `cast/`               |
| Class declarations, inheritance, abstract/final/value | `ClassTestSuite`     | `class/`              |
| Interfaces, default methods, multi-impl | `InterfaceTestSuite`         | `interface/`          |
| Generics on classes/methods/interfaces | `GenericsTestSuite`           | `generics/`           |
| Arrays (literals, multi-dim, `T[]`)    | `ArrayTestSuite`               | `arrays/`             |
| Collections (HashMap/HashSet/List/Queue/Stack/LinkedList) | `CollectionsTestSuite` | `collections/`  |
| Iterators (`Iterator<T>`, for-each integration) | `IteratorTestSuite`    | `iterator/`           |
| Enhanced for-loop                      | `EnhancedForLoopTestSuite`     | `enhancedFor/`        |
| Lambda / closures                      | `LambdaTestSuite`              | `lambda/`             |
| Streams (`Stream<T>`, terminal ops)    | `StreamTestSuite`              | `stream/`             |
| Control flow (`if/else`, `while`, `for`, ternary) | `ControlFlowTestSuite` | `controlFlow/`        |
| Numeric boundaries, bitwise ops, float specials | `NumericsTestSuite`   | `numerics/`           |
| Method overloading / overriding        | `OverloadingTestSuite`         | `overloading/`        |
| Modifiers (`public/private/static/final/abstract`) | `ModifiersTestSuite` | `modifiers/`          |
| Annotations (`@Override`, `@Throw`, `@Script`) | `AnnotationTestSuite`  | `annotation/`         |
| Type checking (assignability, inference) | `TypeCheckingTestSuite`      | `typeChecking/`       |
| Null safety (`T?`, `??`, safe-nav)     | `NullSafetyTestSuite`          | `nullSafety/`         |
| `async/await`                          | `AwaitTestSuite`               | `await/`              |
| Reflection                             | `ReflectionTestSuite`          | `reflection/`         |
| JSON (`json.parse`, serialization)     | `JsonTestSuite`                | `json/`               |
| String interpolation                   | `StringInterpolationTestSuite` | `interpolation/`      |
| String pool semantics                  | `StringPoolTestSuite`          | `stringpool/`         |
| Imports / circular detection           | `ImportTestSuite`              | `import/`             |
| Standard library                       | `LibraryTestSuite`             | `library/`            |
| Error reporting / syntax errors        | `ErrorTestSuite`               | `error/`              |
| GC behaviour                           | `GCTestSuite`                  | `gc/`                 |
| Escape analysis                        | `EscapeAnalysisTestSuite`      | (in-suite fixtures)   |
| Bytecode optimization passes           | `BytecodeOptimizationTestSuite`| (in-suite fixtures)   |
| Executor isolation                     | `ExecutorIsolationTestSuite`   | (in-suite fixtures)   |
| Native plugin loader                   | `PluginTestSuite`              | `plugin/`             |
| Project / `.mtproj` builds             | `ExeTestSuite`                 | `exe/`                |
| End-to-end integration                 | `IntegrationTestSuite`         | `integration/`        |
| ScriptAPI native bindings              | `ScriptApiNativeTestSuite`     | (callback-driven)     |
| LSP completion logic                   | `CompletionLogicTestSuite`     | (callback-driven)     |
| LSP diagnostics                        | `DiagnosticsTestSuite`         | (callback-driven)     |
| LSP workspace symbols                  | `WorkspaceTestSuite`           | (callback-driven)     |
| Debugger protocol                      | `DebuggerTestSuite`            | (callback-driven)     |
| Package manager (`mtpm`)               | `PackageManagerTestSuite`      | (callback-driven)     |
| Dependency graph                       | `DependencyGraphTestSuite`     | (callback-driven)     |

If a feature spans two suites (e.g. "cast involving generics"), pick the suite whose error path you're most likely to break and add the test there.

## Test types

From `mType/tests/testFramework/TestTypeEnum.hpp`. Pick the narrowest type that fits.

| Type                         | Helper                              | What passes                              |
|------------------------------|-------------------------------------|------------------------------------------|
| `OUTPUT_EXPECTED`            | `addOutputVerificationTest`         | `print` output matches sibling `.expected` byte-for-byte (after trailing-whitespace normalization, line count validated) |
| `ERROR_EXPECTED`             | `addTestFromFile(..., TestType::ERROR_EXPECTED)` | Any exception thrown during compile or run |
| `ERROR_EXPECTED` (pinned)    | `addTestFromFile(name, path, TestType::ERROR_EXPECTED, "substring")` | Throws *and* `what()` contains `"substring"` (MYT-38) |
| `NATIVE_CALLBACK`            | `addCallbackTest(name, bootstrap, lambda)` | C++ callback runs without throwing; bootstrap `.mt` executes first |
| `SCRIPT_INTEROP`             | `addInteropTest`                    | Output-verification variant that exercises cross-script ScriptAPI re-entry |
| `EXE_TEST` / `EXE_GUI_TEST`  | `addExeTest` / `addExeGuiTest`      | `.mtproj` builds to .exe and runs producing expected output |
| `DIRECT_SCRIPT_WITH_PROJECT` | `addDirectScriptWithProjectTest`    | Output-verification with the script's ambient `.mtproj` aliases merged (MYT-310) |
| `SKIPPED`                    | `addSkippedTest(name, reason)`      | Explicit skip with reason in the report  |

## OUTPUT_EXPECTED fixture format

`.mt` file:
```mt
// Test: <one-line description>
class Foo { ... }

Foo f = new Foo();
print(f.value());

// Expected output:
// 42
```

The `// Expected output:` block is convention — humans read it before opening the `.expected` sibling.

`.expected` file (`<same-name>.expected` in the same directory):
```
42
```

Comparison rules (`TestCase::verifyOutputAgainstExpected`):
- Trailing whitespace stripped from each line before comparison.
- Empty trailing lines tolerated.
- **Line count is validated first.** A mismatch fails with `Expected: N lines, Actual: M lines` — even if content prefixes match.
- After line-count check, full string compare.

## ERROR_EXPECTED fixture format

`.mt` file:
```mt
// Error: <one-line description of what should fail>
class Dog {}
class Cat {}

Dog d = new Dog();
Cat c = (Cat)d;  // Error: incompatible cast
```

No `.expected` file. Any throw passes by default.

**Pin the error message** with the MYT-38 overload when a test must distinguish *which* error fires (e.g. distinguishing a parse error from a type error from a runtime cast error). Pass the required substring as the fourth argument:
```cpp
addTestFromFile("Cast Null To NonNullable Rejected",
                errorPath + "castNullToNonNullable_error.mt",
                TestType::ERROR_EXPECTED,
                "MT-E2007");
```

The test demotes to FAILED with `Expected error containing 'X' but got: <actual>` if the substring doesn't appear.

## Suite registration

Inside `<Name>TestSuite::setupTests()`, group registrations with `// === SECTION ===` comments. The grouping documents intent and makes diffs reviewable. Look at `CastTestSuite.cpp` for the canonical form — primitive casts, object casts, interface casts, generic casts, etc.

When a test for a feature you removed or never supported lingers in the suite, leave a `// REMOVED - <name> (<why>)` comment instead of just deleting. Future agents will otherwise re-add the same test.

## Adding a new suite

When the feature has no home:

1. Create `mType/tests/suites/<Name>TestSuite.{hpp,cpp}`. Copy the shape from `CastTestSuite.hpp` (one `passPath`, one `errorPath` static const string, `setupTests()` override).
2. Add fixture directories `mType/tests/testFiles/<dir>/pass/` and `error/`.
3. Wire into `mType/run/TestUtilities.cpp`:
   - `#include "../tests/suites/<Name>TestSuite.hpp"`
   - Add a branch in `createTestSuite(suiteName)` that returns `std::make_unique<<Name>TestSuite>()`.
   - Add the new suite name to `printAvailableTestSuites()`.
   - If the suite should run in the `--tests` full pass, add it to the iteration list in the same file.
4. Add both `.hpp` and `.cpp` to `premake5.lua` filters (or the relevant `*.vcxproj.filters`) so MSVC picks them up.
5. Update the suite map in this REFERENCE.md so future agents find it.

## Native callback tests

`addCallbackTest(name, bootstrapPath, [](ScriptAPI& api) { ... })`. The bootstrap `.mt` runs first so the callback can reference classes / globals it declares. Pass an empty string for callbacks that need no `.mt` setup. Any uncaught exception from the lambda fails the test.

Use when:
- The test must drive `.mt` code from C++ (e.g. ScriptAPI shape changes, embedding scenarios).
- Verifying internal state (reflection handles, GC counters) that `print` can't reach.

Avoid when a plain `.mt` fixture would do — output verification is simpler and survives ScriptAPI refactors.

## Project and exe tests

- `addExeTest(name, "path/to/file.mtproj")` — builds `.mtproj` to `.exe`, runs it, verifies output against `<mtprojDir>/<name>.expected` (see `ExeTestSuite.cpp` for layout).
- `addExeGuiTest(...)` — same but uses `mtype-launcher-gui` (windowed subsystem). Stdio still flows back through the parent test harness.
- `addDirectScriptWithProjectTest(name, "path/to/script.mt")` — runs the script via the `mType.exe script.mt` code path, which walks upward to find an ambient `.mtproj` and merges its aliases (workspace + `mt_modules` + `<Alias>`). Use for testing the project-resolution layer specifically — plain output-verification tests skip this.

## JIT toggle

CI runs every suite twice: JIT on (default) and `--no-jit`. The runner flips this via `TestSuite::setJitEnabledForAll(bool)` for each pass. Test files do not need anything special — write the fixture once, both passes exercise it.

If a behavior is JIT-sensitive (OSR, ICs, type-quickening, value-class mutation past the OSR threshold), add a hot-loop variant of the test (`*HotLoop` is the naming convention) so the JIT compiles the relevant function. See `cast/pass/methodTypeParamHotLoop.mt` and the value-class OSR canary referenced in memory.

## Common authoring mistakes

- **`.expected` includes a banner line** (e.g. `// Expected:`) that isn't in `print` output → line count mismatch.
- **Two behaviors in one fixture** → first failure masks the second.
- **Using unsupported syntax** (variance annotations, `<?>`, `::`, namespace) → fixture won't parse. `CastTestSuite.cpp` keeps `// REMOVED -` comments listing the dead-end ones — read them first.
- **String comparisons on output that includes runtime addresses or floating-point representation** → flaky. Normalize the test or pick a discriminator that doesn't depend on print formatting.
- **Forgetting to register the fixture.** Run `scripts/find-unregistered-fixtures.ps1` after batch-creating fixtures.
- **Using `>=` / `<=` on strings** → MT-E5005 (see `feedback_string_compare_no_range.md`). Use `indexOf(charset, c) >= 0` for character-class checks.
