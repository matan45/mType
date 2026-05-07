---
title: Imports
sidebar_position: 7
---

# Imports

mType uses file-path-based imports rooted at the project's import search paths (configured in `.mtproj`).

## Wildcard

```mtype
import * from "lib/collections/ArrayList.mt";
```

Imports every public symbol from the target file.

## Selective

```mtype
import { Calculator, divide, subtract } from "selective_import_utils.mt";
```

Imports only the named symbols.

## Aliased

```mtype
import { Int as MyInt } from "lib/primitives/Int.mt";
```

Renames a symbol locally to avoid collision.

## Visibility — `public` and `private`

Top-level declarations control whether they can be imported from other files. The modifier goes on the **declaration**, not on the `import` statement — there is no `public import` or `private import` syntax in mType.

```mtype
// utils/MathCore.mt

public class MathCore {
    public function square(int x): int { return x * x; }
}

public function abs(int n): int {
    return n < 0 ? -n : n;
}

// File-local — can't be imported
private function internalHelper(int x): int {
    return x * 2;
}

public  int[] publicNumbers  = [1, 2, 3];
private int[] privateNumbers = [10, 20];
```

- **Default visibility is public.** A declaration without a modifier behaves the same as `public`.
- **`private` declarations are file-local.** Trying to import them from another file is a compile error.
- The modifier applies to: classes, interfaces, functions, annotations, and top-level variables / arrays / objects.

### Re-exports are NOT transitive

If file `B.mt` imports something from `A.mt`, that symbol does **not** automatically flow through to whoever imports `B.mt`. To expose a chain of types across module boundaries, use **inheritance** — a `public` subclass in `B.mt` extending a class from `A.mt` is visible to `B`'s importers without re-exporting `A`.

```mtype
// A.mt
public class Base {
    public function hello(): string { return "hi"; }
}

// B.mt
import { Base } from "./A.mt";
public class Mid extends Base { }   // importers of B see Mid (and its inherited hello)
```

The same rule applies to private symbols: even if `B.mt` imports a private symbol from `A.mt` directly, no third file can reach it through `B`.

## Resolution

- The path string is resolved against the project's import search paths (`<importPaths>` in `.mtproj`).
- The default search root is the project's source directory; `lib/...` paths resolve against the bundled standard library.
- Imports always target a single `.mt` file. There are no package-style multi-file imports.

## Cycles

The compiler detects circular imports and reports them with a chain trace. Re-organize your modules to break the cycle.

## Example: a Multi-file Project

```text
src/
  main.mt
  models/
    User.mt
    Post.mt
```

`main.mt`:

```mtype
import { User } from "models/User.mt";
import { Post } from "models/Post.mt";

function main(): void {
    User u = new User("Alice");
    Post p = new Post(u, "Hello");
    print(p.toString());
}

main();
```

To run multi-file projects, use the project system — see [CLI / Projects](../cli/projects.md).

## See Also

- [CLI / Projects](../cli/projects.md) — `.mtproj` configuration and the `--build` workflow.
- [Standard Library / Overview](../stdlib/overview.md) — what's available under `lib/`.
