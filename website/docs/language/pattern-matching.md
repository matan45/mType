---
title: Pattern Matching
sidebar_position: 10
---

# Pattern Matching

`match` performs typed dispatch on a value, with each `case Type binding -> { ... }` arm matching when the runtime type is compatible.

## Basic Form

```mtype
match (s) {
    case Circle c -> {
        print("Circle r=" + c.getRadius() + " area=" + s.area());
    }
    case Rectangle r -> {
        print("Rect " + r.getWidth() + "x" + r.getHeight());
    }
    case Triangle t -> {
        print("Triangle area=" + s.area());
    }
    default -> {
        print("Unknown shape");
    }
}
```

- The case `Circle c` introduces a new local `c` of type `Circle`, bound to `s` if the runtime type matches.
- Arms are checked top-to-bottom; the first compatible match runs.
- A `default ->` arm catches anything not matched above.

## When to Use `match` vs `switch`

| Need | Use |
|---|---|
| Branch on a primitive value (int, string) | [`switch`](control-flow.md#switch--case--default) |
| Branch on the runtime type of an object | `match` |

## Sealed-Class-Style Hierarchies

Combine `match` with abstract base classes to express closed sets of variants:

```mtype
abstract class Shape {
    abstract function area(): float;
}

class Circle    extends Shape { /* ... */ }
class Rectangle extends Shape { /* ... */ }
class Triangle  extends Shape { /* ... */ }

function describe(Shape s): void {
    match (s) {
        case Circle c    -> print("circle");
        case Rectangle r -> print("rect");
        case Triangle t  -> print("tri");
        default          -> print("other");
    }
}
```

## See Also

- [Classes](classes.md) — abstract classes and inheritance.
- [Control Flow](control-flow.md#switch--case--default) — value-based `switch`.
