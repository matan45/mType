---
title: Generics
sidebar_position: 4
---

# Generics

Generic classes, interfaces, and methods let you write code that works over many types without duplication.

## Generic Class

```mtype
class Box<T> {
    T value;

    public function setValue(T newValue): void {
        value = newValue;
    }

    public function getValue(): T {
        return value;
    }
}
```

Instantiate with an explicit type argument:

```mtype
Box<Int> intBox = new Box<Int>();
intBox.setValue(new Int(42));
Int result = intBox.getValue();
```

Multiple type parameters use comma-separated syntax: `class Pair<A, B> { ... }`.

## Generic Interface

```mtype
interface Iterable<T> {
    function iterator(): Iterator<T>;
}

interface Iterator<T> {
    function hasNext(): bool;
    function next(): T;
}
```

## Generic Method

A method can introduce its own type parameter, independent of the class:

```mtype
class Utils {
    public static function <T> first(ArrayList<T> list): T {
        return list.get(0);
    }
}
```

## Bounded Generics

Constrain a type parameter with `extends`:

```mtype
class SortedList<T extends Comparable<T>> {
    // T is guaranteed to provide a compare(T) method
}
```

## Type Inference Caveats

- mType requires explicit type arguments at instantiation: `new ArrayList<Int>()`, not `new ArrayList()`.
- Casts of generic types are conservative — see [Reference / Limitations](../reference/limitations.md).

## See Also

- [Standard Library / Collections](../stdlib/collections.md) — `ArrayList<T>`, `HashMap<K, V>`, `Stream<T>`.
- [Pattern Matching](pattern-matching.md) — typed dispatch with generics.
