---
title: Projects & Workspaces
sidebar_position: 2
---

# Projects and Workspaces

For anything beyond a single script, mType uses an XML project file (`.mtproj`) and an optional workspace file (`.mtworkspace`).

## `.mtproj`

A project file declares:

- `<includes>` and `<excludes>` — glob patterns for source files.
- `<output>` — output directory for compiled bytecode.
- `<importPaths>` — extra directories to search for imports.
- `<importAliases>` — short names mapped to longer paths.
- `<dependencies>` — package dependencies with name, version range, and git source.

## Bootstrap a Project

```bash
mType --init MyApp "src/**/*.mt"
```

This produces `MyApp.mtproj` with `src/**/*.mt` as an include glob.

Add or remove patterns later:

```bash
mType --add    "tests/**/*.mt" MyApp.mtproj
mType --remove "tests/legacy/**" MyApp.mtproj
```

## Build

```bash
# Compile all sources to .mtc files
mType --build MyApp.mtproj

# Bundle into a single .mtcLib library
mType --build --lib MyApp.mtproj

# Build a standalone native executable
mType --build --exe MyApp.mtproj
```

## Native Executables

`--build --exe` embeds compiled bytecode into a launcher binary (`mtype-launcher`) and emits a single self-contained executable. End users run it without a separate mType install — the launcher is included in release archives.

## Inspecting Dependencies

```bash
mType --deps           MyApp.mtproj
mType --deps --json    MyApp.mtproj
mType --deps --dot     MyApp.mtproj
mType --deps --cycles  MyApp.mtproj
mType --deps --why src/foo.mt MyApp.mtproj
```

## `.mtworkspace`

A workspace lists multiple projects that share a build directory.

```bash
mType --init-workspace MyMonorepo
```

Edit `MyMonorepo.mtworkspace` to add `<members>` referencing each `.mtproj`.

## See Also

- [Package Manager](package-manager.md)
- [CLI Reference](reference.md)
