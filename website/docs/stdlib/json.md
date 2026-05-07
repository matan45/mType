---
title: JSON
sidebar_position: 9
---

# JSON

`lib/json/Json.mt` provides type-driven JSON serialization and deserialization. Unlike key-value JSON libraries, mType's `Json` works directly against your classes — you serialize an object and deserialize back into a typed instance.

`Json` is a static facade — every method is called with `Json::method(...)`.

## Serialize

```mtype
import * from "lib/json/Json.mt";

class User {
    public string name;
    public int    age;
    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

User u = new User("Alice", 30);
string raw = Json::serialize(u);
print(raw);   // {"name":"Alice","age":30}
```

For pretty-printed output:

```mtype
string pretty  = Json::stringify(u);  // multi-line, indented
string compact = Json::compact(u);    // single-line
string fmtted  = Json::format(raw);   // re-format an existing string
```

To control whether static fields appear and to choose pretty/compact in one call:

```mtype
string out = Json::serializeWithOptions(u, /*includeStatic=*/false, /*prettyPrint=*/true);
```

## Deserialize

The deserializer needs to know which class to construct. Pass the class name as a string:

```mtype
string raw = "{\"name\":\"Alice\",\"age\":30}";
User u = Json::deserializeAs(raw, "User");
print(u.name);   // Alice
print(u.age);    // 30
```

If the receiver type is fully inferable at the call site (rare in practice — usually you'll need `deserializeAs`):

```mtype
User u = Json::<User>deserialize(raw);
```

## File I/O

```mtype
// Read JSON from disk and deserialize as User.
User u = Json::readFromFile("/path/to/user.json", "User");

// Serialize and write to disk.
Json::writeToFile("/path/to/out.json", u);

// Pretty-print and include static fields.
Json::writeToFileWithOptions("/path/out-pretty.json", u, /*includeStatic=*/true, /*pretty=*/true);
```

## API Summary

| Method | Returns |
|---|---|
| `Json::serialize<T>(T obj)` | `string` (compact) |
| `Json::serializeWithOptions<T>(T obj, bool includeStatic, bool prettyPrint)` | `string` |
| `Json::stringify<T>(T obj)` | `string` (pretty) |
| `Json::compact<T>(T obj)` | `string` (single-line) |
| `Json::deserialize<T>(string json)` | `T` (uses inferred type) |
| `Json::deserializeAs<T>(string json, string className)` | `T` |
| `Json::format(string json)` | `string` (re-formatted) |
| `Json::readFromFile<T>(string path, string className)` | `T` |
| `Json::writeToFile<T>(string path, T obj)` | `void` |
| `Json::writeToFileWithOptions<T>(string path, T obj, bool includeStatic, bool prettyPrint)` | `void` |

## See Also

- [Networking / JsonApi](net.md#jsonapi-class) — JSON-over-HTTP convenience wrapper.
- [Reflection](reflect.md) — `Json` itself is built on the same metadata exposed by `Class`.
