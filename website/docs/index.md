---
title: Welcome to mType
slug: /
---

# mType Documentation

mType is a statically typed, class-based language with a bytecode VM, an x86-64 JIT, a project / workspace build system, the `mtpm` package manager, a standalone language server, and a VS Code extension.

This documentation covers everything you need to write, build, and ship mType programs.

## Where to Start

- **[Install](getting-started/install.md)** — clone, build, and verify mType on Windows, Linux, or macOS.
- **[Quick Start](getting-started/quick-start.md)** — your first script in under five minutes.
- **[First Program](getting-started/first-program.md)** — a guided walkthrough of a complete mType program.

## What's Covered

- **[Language Reference](language/classes.md)** — primitives, classes, interfaces, generics, lambdas, control flow, imports, annotations, async/await, pattern matching.
- **[Standard Library](stdlib/overview.md)** — collections, streams, exceptions, math, networking, reflection, JSON, the `mtest` testing framework.
- **[CLI Reference](cli/reference.md)** — every flag of the `mType` and `mtpm` executables.
- **[Architecture](architecture/overview.md)** — the compilation pipeline, VM, and JIT.
- **[Contributing](contributing.md)** — repo layout, build setup, and project conventions.

## At a Glance

```mtype
class Greeter {
    private string name;

    public constructor(string name) {
        this.name = name;
    }

    public function greet(): void {
        print($"Hello, {this.name}!");
    }
}

function main(): void {
    Greeter g = new Greeter("World");
    g.greet();
}

main();
```

Save as `hello.mt`, then:

```bash
mType hello.mt
```

## Project Status

mType ships as a source distribution. Builds run on Windows (MSVC v143), Linux (GCC), and macOS (Clang). The compiler, VM, JIT, language server, package manager, and VS Code extension source are all in the [main repository](https://github.com/matan45/mType). The standard library lives in [matan45/mTypeLib](https://github.com/matan45/mTypeLib) and is mirrored under `mType/tests/testFiles/lib/` for the test runner.

Known limitations are documented in [Reference / Limitations](reference/limitations.md).
