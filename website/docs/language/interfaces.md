---
title: Interfaces
sidebar_position: 3
---

# Interfaces

Interfaces declare a contract that classes can implement. They contain only method signatures; methods in interfaces have no body.

## Declaration

```mtype
interface Drawable {
    function draw(): void;
    function getArea(): float;
}
```

## Implementation

```mtype
class Circle implements Drawable {
    private float radius;

    public constructor(float r) {
        this.radius = r;
    }

    public function draw(): void {
        print($"Drawing circle r={this.radius}");
    }

    public function getArea(): float {
        return 3.14 * this.radius * this.radius;
    }
}
```

A class can implement multiple interfaces:

```mtype
class Square implements Drawable, Comparable<Square> {
    // ...
}
```

## Polymorphism

```mtype
Drawable d = new Circle(5.0);
d.draw();
print(d.getArea());
```

The static type of `d` is `Drawable`; the runtime dispatches to `Circle.draw` and `Circle.getArea`.

## Generic Interfaces

Interfaces can take type parameters:

```mtype
interface Comparator<T> {
    function compare(T a, T b): int;
}

class IntComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.compareTo(b);
    }
}
```

The standard library ships several functional / SAM interfaces — see [Stream API](../stdlib/stream.md) and [Functional Interfaces](../stdlib/overview.md).

## Default Implementations

Interfaces in mType are pure contracts — they cannot provide method bodies. Use an abstract class if you need to share implementation across implementers.

## See Also

- [Classes](classes.md) for the `extends` form
- [Lambdas](lambdas.md) — lambdas implement single-abstract-method interfaces
