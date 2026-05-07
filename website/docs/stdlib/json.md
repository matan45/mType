---
title: JSON
sidebar_position: 9
---

# JSON

`lib/json/Json.mt` provides JSON parsing and serialization.

## Parsing

```mtype
import * from "lib/json/Json.mt";

string raw = "{\"name\": \"Alice\", \"age\": 30}";
Json data = Json.parse(raw);

print(data.getString("name"));  // Alice
print(data.getInt("age"));      // 30
```

## Serialization

```mtype
import * from "lib/json/Json.mt";

Json obj = Json.object();
obj.putString("name", "Alice");
obj.putInt("age", 30);
print(obj.toString());  // {"name":"Alice","age":30}
```

## Arrays

```mtype
Json arr = Json.array();
arr.addString("a");
arr.addString("b");
print(arr.toString());  // ["a","b"]
```

## Querying

| Method | Returns |
|---|---|
| `getString(key)` | `string` |
| `getInt(key)` | `int` |
| `getFloat(key)` | `float` |
| `getBool(key)` | `bool` |
| `getArray(key)` | `Json` (array) |
| `getObject(key)` | `Json` (nested object) |
| `containsKey(key)` | `bool` |

## See Also

- [Networking / JSON-over-HTTP](net.md#json-over-http)
