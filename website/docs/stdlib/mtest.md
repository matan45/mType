---
title: mtest Testing Framework
sidebar_position: 10
---

# `mtest` — Testing Framework

`lib/mtest/` is the built-in testing framework. Tests look familiar to anyone who has used JUnit or Jest.

## A First Test

```mtype
import * from "lib/mtest/Mtest.mt";
import * from "lib/mtest/Assertions.mt";
import * from "lib/mtest/annotations/Test.mt";

class CalculatorTest {
    @Test
    public function addition_works(): void {
        Assertions.assertEquals(2 + 2, 4);
    }

    @Test
    public function subtraction_works(): void {
        Assertions.assertEquals(5 - 3, 2);
    }
}

function main(): void {
    Mtest.run(new CalculatorTest());
}

main();
```

Run it like any other script:

```bash
mType CalculatorTest.mt
```

## Lifecycle Annotations

```mtype
import * from "lib/mtest/annotations/Lifecycle.mt";
import * from "lib/mtest/annotations/Test.mt";

class FixtureTest {
    @BeforeAll
    public static function setupClass(): void { }

    @BeforeEach
    public function setupCase(): void { }

    @Test
    public function example(): void { /* ... */ }

    @AfterEach
    public function teardownCase(): void { }

    @AfterAll
    public static function teardownClass(): void { }
}
```

## Assertions

`Assertions` (in `lib/mtest/Assertions.mt`) provides:

- `assertEquals(actual, expected)`
- `assertTrue(cond)`, `assertFalse(cond)`
- `assertNull(value)`, `assertNotNull(value)`
- `assertThrows(ExceptionClass, ThrowingRunnable)`
- `fail(message)`

## Async Tests

`@Test` works with `async` methods returning `Promise<void>`:

```mtype
class AsyncTest {
    @Test
    public function async fetches_data(): Promise<void> {
        Json result = await JsonApi.get("https://api.example.com/ping");
        Assertions.assertEquals(result.getString("status"), "ok");
    }
}
```

## Examples in the Repo

- `lib/mtest/examples/CalculatorTest.mt`
- `lib/mtest/examples/AsyncAwaitRunnerTest.mt`

## See Also

- [Annotations](../language/annotations.md)
- [Async / Await](../language/async-await.md)
