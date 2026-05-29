---
name: mtype-language-feature-tests
description: Author tests that exercise mType language features end-to-end through the C++ test harness in mType/tests/. Use when adding coverage for a language feature, writing fixtures for an existing suite (cast, generics, lambda, control flow, etc.), creating .mt pass + .expected pairs, adding ERROR_EXPECTED fixtures (with optional MYT-38 expectedErrorSubstring), wiring a new TestSuite into TestUtilities, or registering native-callback / exe / interop tests.
---

# mType Language Feature Tests

Author fixture-driven tests in `mType/tests/`. Most tests are pairs of files: a `.mt` source under `testFiles/<feature>/pass/` (or `error/`) and an adjacent `.expected` golden output, registered from a `TestSuite` subclass.

## Decide before writing

- **Feature already covered?** Find the owning suite in [REFERENCE.md](REFERENCE.md#suite-map). New tests go there.
- **No suite yet?** Add one (`SuiteName.hpp/.cpp`) **and** wire it into `mType/run/TestUtilities.cpp`. See [REFERENCE.md → Adding a new suite](REFERENCE.md#adding-a-new-suite).
- **What test type?** Default is `OUTPUT_EXPECTED` (golden-file). Use `ERROR_EXPECTED` for tests that must throw — and pin the error with the MYT-38 substring overload when the *reason* matters. See [REFERENCE.md → Test types](REFERENCE.md#test-types).

## Batch workflow (default for coverage tasks)

For coverage work, write **all** fixtures + registrations in one pass — don't slice RED→GREEN per test. (TDD red-green is for *new feature* development; coverage is enumeration.)

1. **Enumerate behaviors** — list each behavior, edge case, and error path the feature should cover. Group them visually the way the existing suite already groups them (the `// === SECTION ===` comments in `CastTestSuite.cpp` are the model).
2. **Scaffold pairs** — for every pass behavior, create `<name>.mt` and `<name>.expected` together. The `.expected` must contain the exact `print` lines the script produces (trailing whitespace and blank lines are normalized; line count is checked).

   Use the helper to avoid typos:
   ```
   powershell -ExecutionPolicy Bypass -File .Codex/skills/mtype-language-feature-tests/scripts/new-fixture-pair.ps1 `
       -Feature cast -Variant pass -Name castNestedGenericTriple
   ```
3. **Write `.mt` sources** — small, focused, one behavior per file. End with a `// Expected output:` block matching the `.expected` content (convention in existing fixtures; humans read this before opening the sibling file).
4. **Write error fixtures** — under `testFiles/<feature>/error/`. No `.expected` needed; any throw passes by default. Pin the error message with `expectedErrorSubstring` when the test is specifically about which error fires.
5. **Register all of them in the suite `.cpp`** — group with `// === SECTION ===` comments matching step 1. Use `addOutputVerificationTest` for pass, `addTestFromFile(..., TestType::ERROR_EXPECTED)` for error.
6. **Verify nothing is orphaned**:
   ```
   powershell -ExecutionPolicy Bypass -File .Codex/skills/mtype-language-feature-tests/scripts/find-unregistered-fixtures.ps1 -Feature cast
   ```
   Exit 0 = every `.mt` under `testFiles/<feature>/` is registered. Exit 1 = list of orphans.
7. **Hand the build to the user** — they build manually (`runPremake.bat` + MSVC) and report results. Do not run `premake5` or `msbuild` yourself.

## Authoring rules

- **Use only features the language actually supports.** `CastTestSuite.cpp` is full of `// REMOVED -` comments for tests that assumed unsupported syntax (variance annotations, wildcards `<?>`, method references `::`, namespace keyword). Before writing a fixture that uses an exotic construct, grep an existing pass test to confirm the syntax compiles.
- **One behavior per `.mt`.** A fixture that tests three things gives one bit of signal when it fails. Split.
- **`.expected` matches `print` output exactly.** No trailing whitespace per line; blank trailing lines are tolerated. Line count is validated separately from content, so a missing line gives a precise error.
- **Suite `.cpp` files are exempt from the 500-line limit** (test infrastructure, registration-heavy). Don't split a suite to stay under it.
- **Don't run the build.** The user builds and reports back. See `feedback_user_runs_build.md` in memory.
- **JIT divergence.** All suites run twice in CI: JIT on (default) and `--no-jit`. If a behavior is JIT-sensitive (OSR, ICs, type-quickening, value-class mutation), add it explicitly — see `project_myt346_value_class_osr_divergence.md` for the canary pattern.

## When the default OUTPUT_EXPECTED pattern doesn't fit

- **Native ScriptAPI callbacks** (driving `mt` code from C++): `addCallbackTest`. See [REFERENCE.md → Native callback tests](REFERENCE.md#native-callback-tests).
- **Cross-script interop / project-aware scripts / .exe builds**: `addInteropTest`, `addDirectScriptWithProjectTest`, `addExeTest`, `addExeGuiTest`. See [REFERENCE.md → Project and exe tests](REFERENCE.md#project-and-exe-tests).
- **Skip when a feature is gated off**: `addSkippedTest(name, reason)` — explicit skip with reason beats silent omission.
