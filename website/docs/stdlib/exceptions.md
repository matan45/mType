---
title: Exceptions
sidebar_position: 5
---

# Exceptions

`lib/exceptions/` defines the exception hierarchy.

## Hierarchy

```text
Exception
├── RuntimeException
│   ├── IllegalArgumentException
│   ├── IndexOutOfBoundsException
│   ├── NullPointerException
│   └── ClassNotFoundException
└── (your custom exception classes)
```

All exceptions extend `Exception` and carry a message accessible via `getMessage()`.

## Throwing

```mtype
import * from "lib/exceptions/IllegalArgumentException.mt";

if (input == "") {
    throw new IllegalArgumentException("input cannot be empty");
}
```

## Catching

```mtype
import * from "lib/exceptions/Exception.mt";

try {
    risky();
} catch (Exception e) {
    print("error: " + e.getMessage());
}
```

A single `try` can have multiple `catch` clauses for different exception types; the first compatible one runs.

## Custom Exceptions

```mtype
import * from "lib/exceptions/Exception.mt";

class ValidationException extends Exception {
    public constructor(string message): super(message) { }
}
```

Declare what a method throws using `@Throw`:

```mtype
@Throw(exceptions = [ValidationException])
public function validate(string input): bool { ... }
```

## Network Exceptions

`lib/net/exceptions/` provides network-specific subclasses:

- `NetworkException`
- `ConnectionException`
- `DnsException`
- `HttpException`
- `TimeoutException`

See [Network](net.md) for how they're raised.

## See Also

- [Language / Control Flow](../language/control-flow.md#try--catch--finally)
- [Annotations](../language/annotations.md) — `@Throw`
