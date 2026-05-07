---
title: Overview
sidebar_position: 1
---

# Standard Library Overview

The mType standard library is distributed as a separate repo: [matan45/mTypeLib](https://github.com/matan45/mTypeLib). Its sources are mirrored under `mType/tests/testFiles/lib/` so the test runner can import them.

Imports against the standard library look like:

```mtype
import * from "lib/collections/ArrayList.mt";
import { Int } from "lib/primitives/Int.mt";
import * from "lib/exceptions/Exception.mt";
```

## Modules

| Group | Modules |
|---|---|
| **Core** | `Object`, `Iterable`, `Iterator`, `Function`, `BiFunction`, `Consumer`, `Predicate`, `Comparator`, `BinaryOperator` |
| **Primitives** | `Int`, `Float`, `Bool`, `String`, `Box<T>` |
| **Collections** | `ArrayList<T>`, `HashMap<K,V>`, `HashSet<T>`, `LinkedList<T>`, `Queue<T>`, `Stack<T>` |
| **Stream API** | `Stream<T>` with `filter`, `map`, `flatMap`, `distinct`, `sorted`, `limit`, `skip`, `reduce`, `forEach` |
| **Exceptions** | `Exception`, `RuntimeException`, `IllegalArgumentException`, `IndexOutOfBoundsException`, `NullPointerException`, `ClassNotFoundException` |
| **Math** | `Vec2f`, `Vec3f`, `Vec4f`, `Matrix3f`, `Matrix4f`, `Quaternion`, `Random` |
| **Network** | `Http`, `HttpRequest`, `HttpResponse`, `TcpSocket`, `TcpServer`, `JsonApi` |
| **Reflection** | `Class`, `Method`, `Field`, `Constructor`, `Annotation` |
| **JSON** | `Json` |
| **Testing — `mtest`** | `TestSuite`, `TestRunner`, `Assertions`, lifecycle annotations (`@Test`, `@BeforeEach`, …) |

Detailed pages:

- [Primitives](primitives.md) — `Int`, `Float`, `Bool`, `String`, `Box<T>`
- [Collections](collections.md) — `ArrayList`, `HashMap`, `HashSet`, `LinkedList`, `Queue`, `Stack`
- [Stream API](stream.md)
- [Exceptions](exceptions.md)
- [Math](math.md)
- [Networking](net.md)
- [Reflection](reflect.md)
- [JSON](json.md)
- [`mtest` Testing Framework](mtest.md)
