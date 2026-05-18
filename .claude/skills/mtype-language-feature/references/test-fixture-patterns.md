# mType Test Fixture Patterns

Prefer existing suite and fixture conventions. Search for a similar feature before adding a new folder or test harness.

## Fixture Layout

- Script fixtures live under `mType/tests/testFiles/`.
- Most feature areas split behavior into `pass` and `error` fixtures.
- Passing scripts that assert stdout usually have paired `.expected` files.
- Bug repros under `mType/tests/testFiles/bugs/` are intentionally not registered until promoted.
- Standard library fixtures are mirrored under `mType/tests/testFiles/lib/` for the test runner.

## Suite Registration

- C++ suites live under `mType/tests/suites/`.
- Add a fixture to the closest existing suite rather than creating a new suite for one behavior.
- Use the feature area as the test name vocabulary: class, interface, generics, typeChecking, integration, diagnostics, packageManager, workspace, etc.

## Test Choice

- Use a `.mt` fixture when behavior is observable through the language CLI.
- Use a suite-level C++ test when the behavior is internal but public to a subsystem, such as bytecode optimization or executor isolation.
- Use language-server tests when editor analysis, completion, hover, references, formatting, diagnostics, or semantic tokens are the public behavior.

## Runtime Parity

For features that execute code:

- Run the relevant test with JIT enabled.
- Run the same relevant path with `--no-jit` when bytecode, VM, values, calls, async, exceptions, or type metadata are affected.
- Use `.expected` output from the interpreter reference when validating JIT behavior.

## Error Behavior

- Add an error fixture when a new syntax or semantic rule rejects code.
- Assert the intended diagnostic class/message pattern if the local suite supports it.
- Avoid tests that only prove "some error happened" when the feature has a defined failure mode.
