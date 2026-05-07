---
title: Arrays
sidebar_position: 11
---

# Arrays

Arrays are fixed-length, indexed sequences of a single element type. They're a built-in language feature, separate from the dynamic `ArrayList<T>` in the standard library — when you need a growable sequence, reach for `ArrayList<T>` instead.

## Declaration

Two ways to create an array:

```mtype
// 1. Allocate a fixed-size array; elements default to zero / null.
int[]    nums   = new int[10];
string[] names  = new string[3];

// 2. Initialize from a literal.
int[]    primes = [2, 3, 5, 7, 11];
string[] words  = ["hello", "world"];
```

The element type sits before the `[]`. Any type works as an element: primitives, classes, interfaces, generics, lambdas (via SAM-interface types).

## Indexing

Zero-based, with bounds checking:

```mtype
int[] a = [10, 20, 30];
print(a[0]);            // 10
a[2] = 99;
print(a[2]);            // 99
```

Out-of-range access throws `IndexOutOfBoundsException`.

## `length`

Every array exposes a `length` property:

```mtype
int[] a = [1, 2, 3, 4, 5];
for (int i = 0; i < a.length; i++) {
    print(a[i]);
}
```

## Iteration

```mtype
int[] a = [1, 2, 3, 4, 5];

// Indexed
for (int i = 0; i < a.length; i++) {
    print(a[i]);
}

// Enhanced for (for-each)
for (int x : a) {
    print(x);
}
```

The for-each form works on any array regardless of element type.

## Multi-Dimensional Arrays

Multi-dim arrays are arrays-of-arrays:

```mtype
int[][] matrix = [[1, 2, 3],
                  [4, 5, 6],
                  [7, 8, 9]];

for (int i = 0; i < matrix.length; i++) {
    for (int j = 0; j < matrix[i].length; j++) {
        print(matrix[i][j]);
    }
}
```

You can also allocate them shape-first:

```mtype
int[][] grid = new int[3][];
for (int i = 0; i < grid.length; i++) {
    grid[i] = new int[3];
}
```

:::note
Sub-arrays in `K[][]` are views, not standalone arrays you can swap. Assigning a whole row (`grid[i] = newRow`) raises a runtime error in current builds — replace elements individually.
:::

## Arrays of Class / Interface Types

Class- and interface-typed arrays are first-class:

```mtype
interface Drawable {
    function draw(): string;
}

Drawable[] shapes = new Drawable[3];
shapes[0] = new Circle(5);
shapes[1] = new Rectangle(10, 20);
shapes[2] = new Circle(3);

for (int i = 0; i < shapes.length; i++) {
    print(shapes[i].draw());
}
```

A subclass instance can be stored in a base-class slot — `Animal[] a = new Animal[3]; a[0] = new Dog(...);` works because each element assignment is an ordinary upcast.

## Array Reference Invariance

While *element* assignment respects subtyping, *whole-array* references are **invariant**:

```mtype
Dog[] dogs = new Dog[2];
Animal[] animals = dogs;      // ✗ rejected — array references are invariant
```

This is intentional — it prevents writes through an `Animal[]` alias from depositing a non-`Dog` into the underlying `Dog[]`. To pass elements into a base-typed slot, copy them individually:

```mtype
Animal[] animals = new Animal[dogs.length];
for (int i = 0; i < dogs.length; i++) {
    animals[i] = dogs[i];      // ✓ element-wise upcast
}
```

## Arrays as Parameters and Return Values

Arrays can be passed and returned like any other reference type:

```mtype
function sum(int[] xs): int {
    int total = 0;
    for (int x : xs) { total = total + x; }
    return total;
}

function range(int n): int[] {
    int[] out = new int[n];
    for (int i = 0; i < n; i++) { out[i] = i; }
    return out;
}
```

Mutations made by the callee are visible to the caller — array values are passed by reference, not copied.

## When to Use What

| Need | Use |
|---|---|
| Fixed-size, low-overhead, primitive-heavy | Built-in array (`int[]`, `float[]`) |
| Growable, with `add` / `remove` / `forEach` | [`ArrayList<T>`](../stdlib/collections.md) |
| Streaming / functional pipelines | [`Stream` API](../stdlib/stream.md) on top of either |

## See Also

- [Standard Library / Collections](../stdlib/collections.md) — `ArrayList<T>`, `HashMap<K,V>`, etc.
- [Generics](generics.md)
- [Control Flow](control-flow.md) — for/for-each loops
