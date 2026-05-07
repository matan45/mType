---
title: Annotations
sidebar_position: 8
---

# Annotations

Annotations attach metadata to declarations. mType ships several built-in annotations and lets you define your own.

## Built-in Annotations

| Annotation | Purpose |
|---|---|
| `@Override` | Declares the method overrides a parent method (compile-time check). |
| `@Throw(exceptions = [...])` | Declares the checked exceptions a method may throw. |
| `@Script` | Marks a class for embedding from C++ (FFN / native interop). |
| `@Retention(...)` | Sets the lifetime of a user-defined annotation: `SOURCE`, `CLASS`, or `RUNTIME`. |
| `@Target([...])` | Restricts where a user-defined annotation can be applied: `CLASS`, `METHOD`, `FIELD`, `PARAMETER`, etc. |
| `@Test`, `@BeforeAll`, `@AfterAll`, `@BeforeEach`, `@AfterEach` | Lifecycle annotations from the [`mtest`](../stdlib/mtest.md) framework. |

## `@Throw`

Declares the checked exceptions a method may throw.

```mtype
class ValidationException extends Exception {
    public constructor(string message): super(message) { }
}

class UserService {
    @Throw(exceptions = [ValidationException])
    public function validateUser(string username): bool {
        if (username == "") {
            throw new ValidationException("empty");
        }
        return true;
    }
}
```

The `exceptions` array must be supplied — even for a single exception class.

## `@Script`

Tags a class for native interop. The C++ host can locate, instantiate, and call methods on `@Script` classes via the embedded API.

```mtype
@Script
public class GameLogic {
    public function onStart(): void { print("started"); }
    public function onTick(): void { /* ... */ }
}
```

Use `mType --find-script-classes <file.mt>` to list `@Script` classes in a script.

See [Native Interop (FFN)](../cli/native-interop.md) for the full embedding API and the C++ side of the bridge.

## `@Override`

```mtype
class Parent {
    public function greet(): void { print("hi"); }
}

class Child extends Parent {
    @Override
    public function greet(): void { print("hey"); }
}
```

If the method does **not** override anything, the compiler raises an error.

## User-Defined Annotations

Declare with `annotation`:

```mtype
@Retention(RUNTIME)
@Target([METHOD])
annotation Logged {
    string level = "info";
}

annotation Timeout {
    int ms;
}
```

Apply with `@Name(field = value)` named-argument syntax:

```mtype
class Service {
    @Timeout(ms = 5000)
    public function ping(): void {
        print("ping");
    }
}
```

Field types include `int`, `float`, `bool`, `string`, arrays, and class references.

## Reading Annotations at Runtime

`@Retention(RUNTIME)` annotations can be inspected via reflection:

```mtype
import * from "lib/reflect/Class.mt";
import * from "lib/reflect/Method.mt";
import * from "lib/reflect/Annotation.mt";

Class c = Class::forName("Service");
Method m = c.getDeclaredMethod("ping", 0);
Annotation? t = m.getAnnotation("Timeout");
if (t != null) {
    print(t.getInt("ms"));
}
```

## See Also

- [Reflection](../stdlib/reflect.md)
- [`mtest`](../stdlib/mtest.md) — `@Test` and lifecycle annotations.
