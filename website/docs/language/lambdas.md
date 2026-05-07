---
title: Lambdas
sidebar_position: 5
---

# Lambdas

Lambdas are anonymous functions that implement single-abstract-method (SAM) interfaces.

## Basic Syntax

```mtype
interface Function {
    function apply(int x): int;
}

Function doubler = x -> x * 2;
print("5 doubled is: " + doubler.apply(5));
```

The arrow form is `(args) -> expression` for a single expression, or `(args) -> { ... return value; }` for a block.

## Capturing Variables

Lambdas capture variables from the enclosing scope by reference. Modifications made to captured variables outside the lambda are visible inside:

```mtype
int[] data = [1, 2, 3, 4];
ArrayOperation sumCalc = () -> {
    int total = 0;
    for (int i = 0; i < data.length; i++) {
        total = total + data[i];
    }
    return total;
};
int sum = sumCalc.execute();
```

## Multi-arg Lambdas

```mtype
interface BiFunction {
    function apply(int a, int b): int;
}

BiFunction add = (a, b) -> a + b;
print(add.apply(2, 3));  // 5
```

## Lambdas with the Stream API

```mtype
import * from "lib/primitives/Int.mt";
import * from "lib/collections/ArrayList.mt";

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));

Int sum = numbers.stream()
    .filter(x -> x > 2)
    .map(x -> x * 10)
    .reduceWithIdentity(0, (a, b) -> a + b);

print(sum);
```

## Functional Interfaces

The standard library provides ready-made SAM interfaces:

- `Function<T, R>` — `apply(T): R`
- `BiFunction<A, B, R>` — `apply(A, B): R`
- `Consumer<T>` — `accept(T): void`
- `Predicate<T>` — `test(T): bool`
- `Comparator<T>` — `compare(T, T): int`
- `BinaryOperator<T>` — `apply(T, T): T`

## See Also

- [Interfaces](interfaces.md) — what lambdas conform to.
- [Standard Library / Stream](../stdlib/stream.md) — chained operations using lambdas.
