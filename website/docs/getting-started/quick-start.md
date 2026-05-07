---
title: Quick Start
sidebar_position: 2
---

# Quick Start

Get from zero to a running program in five minutes.

## 1. Write a Script

Save this as `hello.mt`:

```mtype
import * from "lib/collections/ArrayList.mt";

class Person {
    private string name;
    private int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function toString(): string {
        return $"{this.name} (age {this.age})";
    }
}

function main(): void {
    ArrayList<Person> people = new ArrayList<Person>();
    people.add(new Person("Alice", 30));
    people.add(new Person("Bob", 25));
    for (Person p : people) {
        print(p.toString());
    }
}

main();
```

## 2. Run It

Direct interpretation:

```bash
mType hello.mt
```

Output:

```
Alice (age 30)
Bob (age 25)
```

## 3. Compile to Bytecode

mType compiles `.mt` source to `.mtc` bytecode and can re-run the bytecode without touching the source:

```bash
mType --compile hello.mt
mType --run-cached hello.mtc
```

The bytecode is portable across mType installs of the same version.

## 4. Disable JIT (Optional)

The x86-64 JIT runs by default. To disable it (for benchmarking or debugging the interpreter loop):

```bash
mType --no-jit hello.mt
```

## 5. Inspect Statistics

```bash
mType --gc-stats  hello.mt
mType --jit-stats hello.mt
mType --profile   hello.mt
```

## What's Next

- **[First Program](first-program.md)** — a guided walkthrough of a richer example.
- **[Language Reference](../language/classes.md)** — full syntax tour.
- **[CLI Reference](../cli/reference.md)** — every flag, grouped by purpose.
