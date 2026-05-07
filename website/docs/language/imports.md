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
