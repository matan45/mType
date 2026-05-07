---
title: Control Flow
sidebar_position: 6
---

# Control Flow

mType supports the usual structured control statements plus pattern matching (covered separately).

## `if` / `else`

```mtype
if (x > 0) {
    print("positive");
} else if (x < 0) {
    print("negative");
} else {
    print("zero");
}
```

## Loops

### `for`

```mtype
for (int i = 0; i < 5; i++) {
    print(i);
}
```

### `while`

```mtype
int n = 0;
while (n < 5) {
    n = n + 1;
}
```

### `do` / `while`

```mtype
int dk = 0;
do {
    dk = dk + 1;
} while (dk < 2);
```

### Enhanced `for` (for-each)

Iterate any `Iterable<T>`:

```mtype
for (String word : list) {
    print(word);
}
```

## `break` and `continue`

```mtype
for (int i = 0; i < 10; i++) {
    if (i == 5) { break; }
    if (i % 2 == 0) { continue; }
    print(i);
}
```

## `switch` / `case` / `default`

```mtype
int x = 2;
switch (x) {
    case 1:
        print(100);
        break;
    case 2:
        print(200);
        break;
    case 3:
        print(300);
        break;
    default:
        print(999);
        break;
}
```

`break` is required to prevent fall-through. For typed dispatch, use [Pattern Matching](pattern-matching.md).

## `try` / `catch` / `finally`

```mtype
import * from "lib/exceptions/Exception.mt";

function test(): int {
    try {
        throw new Exception("Test exception");
    } catch (Exception e) {
        print("In catch: " + e.getMessage());
        return 30;
    } finally {
        print("In finally");
        return 40;  // overrides the catch return
    }
}
```

A `finally` block always executes; its `return` overrides earlier returns from `try` or `catch`.

Custom errors subclass `Exception` (or one of `RuntimeException`, `IllegalArgumentException`, `IndexOutOfBoundsException`, `NullPointerException`, `ClassNotFoundException`).

```mtype
class ValidationException extends Exception {
    public constructor(string message): super(message) { }
}
```

## `throw`

```mtype
if (input == null) {
    throw new IllegalArgumentException("input cannot be null");
}
```

Methods that throw checked exceptions should declare them with `@Throw`:

```mtype
@Throw(exceptions = [ValidationException])
public function validate(string input): bool { ... }
```

## See Also

- [Pattern Matching](pattern-matching.md)
- [Annotations](annotations.md) — `@Throw`
- [Standard Library / Exceptions](../stdlib/exceptions.md)
