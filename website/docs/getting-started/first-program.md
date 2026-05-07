---
title: First Program
sidebar_position: 3
---

# Your First Program

A guided tour of a complete mType program. We'll write a tiny todo-list app that exercises classes, generics, lambdas, the standard library, and string interpolation.

## The Program

Save as `todo.mt`:

```mtype
import * from "lib/collections/ArrayList.mt";

class Todo {
    private string title;
    private bool done;

    public constructor(string title) {
        this.title = title;
        this.done = false;
    }

    public function complete(): void {
        this.done = true;
    }

    public function toString(): string {
        string mark = this.done ? "[x]" : "[ ]";
        return $"{mark} {this.title}";
    }
}

function main(): void {
    ArrayList<Todo> todos = new ArrayList<Todo>();
    todos.add(new Todo("Read the docs"));
    todos.add(new Todo("Write a program"));
    todos.add(new Todo("Ship it"));

    todos.get(0).complete();

    for (Todo t : todos) {
        print(t.toString());
    }
}

main();
```

Run it:

```bash
mType todo.mt
```

Output:

```
[x] Read the docs
[ ] Write a program
[ ] Ship it
```

## What This Demonstrates

| Feature | Where |
|---|---|
| **Imports** | `import * from "lib/collections/ArrayList.mt";` — pulls a generic `ArrayList<T>` from the standard library. |
| **Class declaration** | `class Todo { ... }` — fields, constructor, methods. |
| **Access modifiers** | `private string title;` and `public constructor(...)`. |
| **Generic instantiation** | `new ArrayList<Todo>()` — explicit type argument. |
| **String interpolation** | `$"{mark} {this.title}"` — `$"..."` strings inline expressions. |
| **Enhanced `for`** | `for (Todo t : todos) { ... }` — iterate any `Iterable<T>`. |
| **Standard library** | `ArrayList<T>` from `lib/collections/`. |

## Next Steps

- Read the [Language Reference](../language/classes.md) for the full syntax.
- Browse the [Standard Library](../stdlib/overview.md) for what's available out of the box.
- Try the [Async/Await](../language/async-await.md) page for a concurrency example.
