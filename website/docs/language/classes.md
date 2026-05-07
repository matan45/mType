---
title: Classes
sidebar_position: 2
---

# Classes

Classes are mType's core unit of organization.

## Declaration

```mtype
class Person {
    private string name;
    private int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function greet(): void {
        print($"Hi, I'm {this.name}");
    }
}
```

- Fields go inside the class body, with type annotations.
- The constructor is named `constructor` (no `function` keyword).
- Methods are declared with `function name(args): ReturnType { ... }`.

## Access Modifiers

| Modifier | Visibility |
|---|---|
| `public` | Accessible everywhere |
| `private` | Only inside the declaring class |
| `protected` | Inside the class and subclasses |

A field or method without a modifier defaults to `public`.

## Inheritance

```mtype
abstract class Shape {
    abstract function getArea(): float;

    public function describe(): void {
        print("I am a shape");
    }
}

class Circle extends Shape {
    private float radius;

    constructor(float r) {
        radius = r;
    }

    public function getArea(): float {
        return 3.14 * radius * radius;
    }
}

Circle c = new Circle(5.0);
print("Area: " + c.getArea());
c.describe();
```

- `extends` introduces a parent class.
- `abstract class` cannot be instantiated; `abstract function` has no body and must be implemented by subclasses.
- `super(...)` in a subclass constructor delegates to the parent constructor:

```mtype
class Derived extends Base {
    public constructor(int v): super(v) { }
}
```

## Final

- `final class C` — cannot be extended.
- `final function f()` — cannot be overridden.
- `final` on a local or field — write-once.

## Static

```mtype
class Counter {
    public static int total = 0;

    public static function increment(): void {
        Counter::total = Counter::total + 1;
    }
}

Counter::increment();
print(Counter::total);
```

`static` members belong to the class, not instances. **Access them with the scope operator `::`, not `.`** — `ClassName::member` and `ClassName::method(args)`. The `.` operator is for instance access. This applies to every static reference in the language: standard library factories like `Matrix4f::identity()`, reflection lookups like `Class::forName("Service")`, and any user-defined static method.

## Value Classes

`value class` types have **copy semantics** on assignment — assigning one variable to another produces an independent copy, like a struct in other languages.

```mtype
value class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int { return this.x; }
    public function getY(): int { return this.y; }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

Point p1 = new Point(3, 4);
Point p2 = p1;          // p2 is an independent copy
print(p1.toString());
```

Value classes cannot use `extends` — primitives' Object-subtype membership comes from the type registry, not source-level inheritance.

## Casting

```mtype
class Base { public int value; public constructor(int v) { this.value = v; } }
class Derived extends Base { public constructor(int v): super(v) { } }

Derived d1 = new Derived(10);
Base    b  = (Base) d1;
print(b.value);
```

## Nullable Types

A `?` suffix marks a type nullable:

```mtype
Point? maybe = null;
if (maybe != null) {
    print(maybe.toString());
}
```

## See Also

- [Interfaces](interfaces.md)
- [Generics](generics.md)
- [Reflection](../stdlib/reflect.md)
