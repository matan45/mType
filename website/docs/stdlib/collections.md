---
title: Collections
sidebar_position: 3
---

# Collections

`lib/collections/` provides the core data structures.

| Type | File | Description |
|---|---|---|
| `ArrayList<T>` | `lib/collections/ArrayList.mt` | Dynamic array. |
| `LinkedList<T>` | `lib/collections/LinkedList.mt` | Doubly linked list. |
| `HashMap<K, V>` | `lib/collections/HashMap.mt` | Open-addressing hash map. |
| `HashSet<T>` | `lib/collections/HashSet.mt` | Open-addressing hash set. |
| `Stack<T>` | `lib/collections/Stack.mt` | LIFO stack. |
| `ArrayQueue<T>` | `lib/collections/ArrayQueue.mt` | FIFO queue. |

All collections implement the corresponding interfaces in `lib/interfaces/` (`List<T>`, `Map<K, V>`, `Set<T>`, `Queue<T>`, `Deque<T>`) and `Iterable<T>`.

## `ArrayList<T>`

```mtype
import * from "lib/collections/ArrayList.mt";

ArrayList<string> names = new ArrayList<string>();
names.add("Alice");
names.add("Bob");
names.add("Carol");

print(names.size());           // 3
print(names.get(1));           // Bob
print(names.contains("Carol")); // true

for (string n : names) {
    print(n);
}
```

Common methods: `add`, `remove`, `get`, `set`, `size`, `contains`, `clear`, `forEach`, `iterator`, `stream`.

## `HashMap<K, V>`

```mtype
import * from "lib/collections/HashMap.mt";

HashMap<string, int> ages = new HashMap<string, int>();
ages.put("Alice", 30);
ages.put("Bob", 25);

print(ages.get("Alice"));        // 30
print(ages.containsKey("Bob"));  // true
print(ages.size());              // 2

for (string name : ages.keys()) {
    print($"{name}: {ages.get(name)}");
}
```

Methods: `put`, `get`, `remove`, `containsKey`, `keys`, `values`, `entries`, `size`, `clear`.

## `HashSet<T>`

```mtype
import * from "lib/collections/HashSet.mt";

HashSet<int> seen = new HashSet<int>();
seen.add(1);
seen.add(1);
seen.add(2);
print(seen.size());  // 2 (duplicates ignored)
```

Methods: `add`, `remove`, `contains`, `size`, `clear`, `iterator`.

## `Stack<T>` and `ArrayQueue<T>`

```mtype
import * from "lib/collections/Stack.mt";
import * from "lib/collections/ArrayQueue.mt";

Stack<int> s = new Stack<int>();
s.push(1); s.push(2);
print(s.pop());  // 2

ArrayQueue<string> q = new ArrayQueue<string>();
q.enqueue("first");
q.enqueue("second");
print(q.dequeue());  // first
```

## Iterators and Streams

Every collection exposes an `iterator()` method and a `stream()` method. See [Stream API](stream.md) for chained operations.

## See Also

- [Stream API](stream.md)
- [Language / Generics](../language/generics.md)
