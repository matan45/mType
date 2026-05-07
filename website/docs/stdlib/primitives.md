---
title: Primitives
sidebar_position: 2
---

# Primitive Wrappers

The standard library wraps the four primitive types as classes so they can flow through generic containers and `Object`-typed APIs.

| Class | File | Highlights |
|---|---|---|
| `Int` | `lib/primitives/Int.mt` | integer wrapper with arithmetic, `compareTo` |
| `Float` | `lib/primitives/Float.mt` | float wrapper with arithmetic, `lessThan`, `greaterThan`, `toInt` |
| `Bool` | `lib/primitives/Bool.mt` | boolean wrapper with `and`, `or`, `xor`, `not` |
| `String` | `lib/primitives/String.mt` | string wrapper |
| `Box<T>` | `lib/primitives/Box.mt` | generic single-slot container |

## `Int`

```mtype
import * from "lib/primitives/Int.mt";

Int a = new Int(10);
Int b = new Int(3);

print(a.getValue());        // 10
print(a.add(b).getValue()); // 13
print(a.compareTo(b));      // 1  (a > b)
```

Methods: `getValue() : int`, `compareTo(Int other) : int`, plus arithmetic helpers (`add`, `subtract`, `multiply`, `divide`).

## `Float`

```mtype
import * from "lib/primitives/Float.mt";

Float pi = new Float(3.14);
print(pi.getValue());            // 3.14
print(pi.toInt());               // 3
print(pi.greaterThan(new Float(2.0)));  // true
```

## `Bool`

```mtype
import * from "lib/primitives/Bool.mt";

Bool t = new Bool(true);
Bool f = new Bool(false);
print(t.and(f).getValue());  // false
print(t.or(f).getValue());   // true
print(t.not().getValue());   // false
```

## `String`

```mtype
import * from "lib/primitives/String.mt";

String s = new String("hello");
print(s.getValue());
```

## `Box<T>`

A generic single-slot container. Useful for holding a primitive by reference inside a closure, or for representing optional state.

```mtype
import * from "lib/primitives/Box.mt";

Box<int> counter = new Box<int>(0);
counter.set(counter.get() + 1);
print(counter.get());  // 1
```

## See Also

- [Language / Primitives](../language/primitives.md) — raw primitive types and literals.
