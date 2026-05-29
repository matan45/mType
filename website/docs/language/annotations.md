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
| `@Getter` / `@Setter` | Generate `getX()` / `setX(value)` accessors for every field. |
| `@ToString` | Generate a structural `toString()`. |
| `@NoArgsConstructor` / `@AllArgsConstructor` | Generate a zero-arg / all-fields constructor. |
| `@EqualsAndHashCode` | Request structural `equals()` / `hashCode()`. |
| `@Data` | Bundle of `@Getter` + `@Setter` + `@ToString` + `@EqualsAndHashCode` + a required-args constructor. |
| `@Builder` | Generate a fluent companion builder and a static `builder()` factory. |
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

## Boilerplate Synthesis (Lombok-style)

These compile-time markers tell the compiler to generate common boilerplate —
accessors, constructors, `toString`, `equals`/`hashCode`, and builders — directly
into the class before bytecode generation. The generated members behave exactly
like hand-written ones (they participate in dispatch, overriding, and reflection
as ordinary members), but they are marked synthetic so reflection's
`getDeclaredMethods()` filters them out by default.

Generation always **skips a member you already declared** — write your own
`getName()` or constructor and the matching synthesis is suppressed for it.
Abstract, `value`, and generic classes are skipped entirely.

### `@Getter` / `@Setter`

```mtype
@Getter
@Setter
class Person {
    private string name;
    private int age;
}

Person p = new Person();   // default constructor
p.setName("Bob");
p.setAge(30);
print(p.getName());        // Bob
```

`@Setter` skips `final` fields.

### `@NoArgsConstructor` / `@AllArgsConstructor`

```mtype
@AllArgsConstructor
class Animal {
    protected string name;
}

@AllArgsConstructor
class Dog extends Animal {
    private string breed;
}

Dog d = new Dog("Rex", "Lab");   // inherited `name` forwarded via super(...)
```

`@AllArgsConstructor` walks the parent chain: inherited fields come first in the
parameter list and are forwarded to `super(...)`, then the class's own fields are
assigned.

### `@ToString`

```mtype
@ToString
@AllArgsConstructor
class Point {
    private int x;
    private int y;
}

print(new Point(3, 4).toString());   // Point(x=3, y=4)
```

Object-typed fields are rendered through their own `toString()`.

### `@Data`

`@Data` is shorthand for `@Getter` + `@Setter` + `@ToString` +
`@EqualsAndHashCode` plus a constructor over the `final` fields:

```mtype
@Data
class Pair {
    private final int a;
    private final int b;
}

Pair p1 = new Pair(1, 2);
Pair p2 = new Pair(1, 2);
print(p1.equals(p2));   // true — structural equality
print(p1.toString());   // Pair(a=1, b=2)
```

`equals`/`hashCode` are produced by the structural-equality optimizer pass, so
two instances with equal fields compare equal and hash alike (suitable for
`HashMap`/`HashSet` keys).

### `@Builder`

```mtype
@Builder
@Getter
class Config {
    private int port;
    private string host;
}

Config c = Config::builder()
    .port(8080)
    .host("localhost")
    .build();
```

`@Builder` emits a companion `ConfigBuilder` class with one fluent setter per
field plus `build()`, and a static `Config::builder()` factory. It also ensures
an all-args constructor exists for `build()` to call.

> These markers are compile-time only (no `@Retention`); they are not visible to
> runtime reflection. Synthesis is implemented in the compiler, not in mType
> source — the language has no runtime metaprogramming.

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
