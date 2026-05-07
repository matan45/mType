---
title: Stream API
sidebar_position: 4
---

# Stream API

`lib/stream/Stream.mt` provides a Java-style chainable stream over any `Iterable<T>`.

## Building a Stream

Every collection has a `stream()` method:

```mtype
import * from "lib/primitives/Int.mt";
import * from "lib/collections/ArrayList.mt";

ArrayList<Int> numbers = new ArrayList<Int>();
numbers.add(new Int(1));
numbers.add(new Int(2));
numbers.add(new Int(3));

Stream<Int> s = numbers.stream();
```

## Chaining Operations

```mtype
Int sum = numbers.stream()
    .filter(x -> x > 2)
    .map(x -> x * 10)
    .reduceWithIdentity(0, (a, b) -> a + b);

print(sum);
```

## Operations

| Operation | Kind | Description |
|---|---|---|
| `filter(Predicate<T>)` | intermediate | Keep elements matching the predicate. |
| `map(Function<T, R>)` | intermediate | Transform each element. |
| `flatMap(Function<T, Stream<R>>)` | intermediate | Flatten nested streams. |
| `distinct()` | intermediate | Remove duplicates. |
| `sorted(Comparator<T>)` | intermediate | Sort with a comparator. |
| `limit(int)` | intermediate | Take the first N. |
| `skip(int)` | intermediate | Drop the first N. |
| `reduce(BinaryOperator<T>)` | terminal | Fold without an identity. |
| `reduceWithIdentity(T, BinaryOperator<T>)` | terminal | Fold with an identity. |
| `forEach(Consumer<T>)` | terminal | Side-effect each element. |
| `toList()` | terminal | Collect to an `ArrayList<T>`. |
| `count()` | terminal | Number of elements. |

## Example: Word Count

```mtype
ArrayList<String> words = ...;
HashMap<String, int> counts = new HashMap<String, int>();
words.stream().forEach(w -> {
    if (counts.containsKey(w)) {
        counts.put(w, counts.get(w) + 1);
    } else {
        counts.put(w, 1);
    }
});
```

## See Also

- [Collections](collections.md)
- [Language / Lambdas](../language/lambdas.md)
