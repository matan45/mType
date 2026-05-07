---
title: Primitives
sidebar_position: 1
---

# Primitives

mType has four built-in primitive types and a generic single-slot container.

| Lowercase (raw) | Wrapper class | Notes |
|---|---|---|
| `int` | `Int` | 64-bit signed integer |
| `float` | `Float` | 64-bit IEEE float |
| `bool` | `Bool` | `true` / `false` |
| `string` | `String` | UTF-8 string, interned in the string pool |
| — | `Box<T>` | Generic single-slot container |

The lowercase forms (`int`, `float`, `bool`, `string`) are the raw primitives. The capitalized wrappers (`Int`, `Float`, `Bool`, `String`) are object types that flow through generic containers and `Object`-typed APIs.

## Literals

```mtype
int    n      = 42;
float  pi     = 3.14;
bool   ok     = true;
string greet  = "Hello";
```

Hex literals are written `0x...`. Floats accept exponent notation: `1.5e3`.

## Wrappers

```mtype
import * from "lib/primitives/Int.mt";

Int boxed = new Int(42);
print(boxed.getValue());      // 42
print(boxed.compareTo(new Int(10)));  // 1 (greater)
```

`Int`, `Float`, and `Bool` provide `getValue()`, comparison, and arithmetic methods. `Bool` adds `and`, `or`, `xor`, `not`. `Float` adds `lessThan`, `greaterThan`, and `toInt`.

## `Box<T>`

A generic single-slot holder for any type:

```mtype
import * from "lib/primitives/Box.mt";

Box<string> name = new Box<string>("World");
name.set("mType");
print(name.get());  // mType
```

Useful when you need to pass a primitive by reference into a closure or generic API.

## Conversion

| From → To | How |
|---|---|
| `int` ↔ `float` | Implicit widening on `int → float`; explicit `(int) f` for the reverse. |
| `int` → `Int` | `new Int(n)`. |
| `Int` → `int` | `boxed.getValue()`. |
| anything → `string` | string interpolation `$"{value}"` or `+` concatenation. |

See [Standard Library / Primitives](../stdlib/primitives.md) for full method lists.
