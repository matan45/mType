---
title: mtest Testing Framework
sidebar_position: 10
---

# `mtest` — Testing Framework

`lib/mtest/` is the built-in testing framework. Tests look familiar to anyone who has used JUnit or Jest.

## A First Test

A single import (`Mtest.mt`) brings in everything: lifecycle annotations, assertions, `TestSuite`, and `TestRunner`.

```mtype
import * from "../lib/mtest/Mtest.mt";

class Calculator {
    public constructor() { }

    public function add(int a, int b): int { return a + b; }
    public function divide(int a, int b): int {
        if (b == 0) { throw new Exception("cannot divide by zero"); }
        return a / b;
    }
}

public class CalculatorTest extends TestSuite {
    private Calculator calc;

    public constructor() : super() { }

    @BeforeEach
    public function setUp(): void {
        this.calc = new Calculator();
    }

    @Test
    public function testAdd(): void {
        assertEqual(this.calc.add(2, 2), 4);
    }

    @Test
    public function testAddNegative(): void {
        assertEqual(this.calc.add(-1, 1), 0, "negatives should cancel");
    }

    @Test(expected = "Exception")
    public function testDivideByZero(): void {
        int x = this.calc.divide(1, 0);
    }
}
```

## Running Tests

A separate driver script discovers and runs the suite:

```mtype
import * from "./CalculatorTest.mt";

TestRunner runner = new TestRunner();
runner.addClass("CalculatorTest");
await runner.run();
```

Run it as a script:

```bash
mType runCalculatorTest.mt
```

## Test Class Convention

- The class **must extend `TestSuite`**.
- Constructor body should not initialize fields — use `@BeforeEach` for per-test state. The runner constructs a fresh instance for each `@Test`.
- Discovery is explicit via `runner.addClass("Name")`. There is no auto-scan in v1.

## Assertions

Assertions are **free functions**, not static methods on a class. Call them bare:

```mtype
assertEqual(actual, expected);
assertEqual(actual, expected, "message");
assertNotEqual(actual, other);
assertTrue(condition);
assertFalse(condition);
assertApprox(actualFloat, expectedFloat, tolerance);
assertNull(value);
assertNotNull(value);
assertThrows("ExceptionName", () -> { /* throwing code */ });
fail("explicit failure message");
```

`assertEqual` and `assertNotEqual` are overloaded for `int`, `float`, `bool`, and `string`. `assertApprox` is the float-tolerant comparison.

`assertThrows` matches the thrown exception's `toString()` prefix (the `ClassName: msg` convention used by `lib/exceptions/`). Pass `"Exception"` as a permissive any-exception match.

## Lifecycle Annotations

```mtype
public class FixtureTest extends TestSuite {
    public constructor() : super() { }

    @BeforeAll
    public function setupClass(): void { /* runs once before any @Test */ }

    @BeforeEach
    public function setupCase(): void { /* runs before each @Test */ }

    @Test
    public function example(): void { /* ... */ }

    @AfterEach
    public function teardownCase(): void { /* runs after each @Test */ }

    @AfterAll
    public function teardownClass(): void { /* runs once after all @Tests */ }
}
```

`@BeforeAll` and `@AfterAll` run on a dedicated bootstrap instance (not a true static context) because reflection's `Method.invoke` requires a non-null receiver. Treat them as shared-instance hooks in v1.

## `@Test` Options

```mtype
@Test                              // ordinary test
@Test(expected = "Exception")      // passes only if the body throws
@Disabled(reason = "pending feature")
@Test
public function testNotYet(): void { fail("should not run"); }
```

## Async Tests

`@Test` works with `async` methods returning `Promise<void>`. `await` runs the suspended coroutine on the VM event loop:

```mtype
public class AsyncTest extends TestSuite {
    public constructor() : super() { }

    @Test
    public function async fetches_data(): Promise<void> {
        JsonApi api = new JsonApi("https://api.example.com");
        String body = await api.get("/ping");
        assertNotNull(body);
    }
}
```

## v1 Limitations

- `@Timeout` is parsed but not enforced — no timing native yet.
- `assertThrows` matches by exception name prefix; subclass matching is not supported.
- Tests run sequentially; no parallel execution.
- `runAndExit()` returns 0/1 but does not actually terminate (mType has no `exit` native).

## Examples in the Repo

- `lib/mtest/examples/CalculatorTest.mt` + `runCalculatorTest.mt`
- `lib/mtest/examples/AsyncAwaitRunnerTest.mt` + `runAsyncAwaitRunnerTest.mt`

## See Also

- [Annotations](../language/annotations.md)
- [Async / Await](../language/async-await.md)
