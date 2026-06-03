---
title: Package Manager (mtpm)
sidebar_position: 3
---

# `mtpm` — Package Manager

mType ships with `mtpm`, a git-source package manager. Dependencies are declared in `.mtproj` and resolved from git URLs (e.g. `github:owner/repo`) — there is no central registry yet.

## Common Commands

```bash
mtpm install [project.mtproj]
mtpm add <pkg>@<ver> [--source github:user/repo]
mtpm remove <pkg>
mtpm list
mtpm init <name> <version>
mtpm --help
mtpm --version
```

## Adding a Dependency

```bash
mtpm add Logger@^1.0.0 --source github:owner/Logger
```

This appends a `<dependency>` entry to your `.mtproj` and clones the source into `mt_modules/`.

## Installing

```bash
mtpm install
```

Resolves and pulls every declared dependency, respecting version ranges.

## Listing

```bash
mtpm list
```

Shows the resolved tree of installed packages.

## Authoring a Package

An mType package repo should contain an `mtpkg.json` manifest at the repo root.
Create one with:

```bash
mtpm init mylib 1.0.0
```

This creates a minimal manifest:

```json
{
  "name": "mylib",
  "version": "1.0.0",
  "source": "src"
}
```

The `source` field points to the package source directory. When the package is installed into `mt_modules/`, mType exposes that source directory as an import alias.

For example, an installed package named `mylib` with `"source": "src"` can be imported by consumers through the generated `@mylib` alias.

### Manifest Fields

```json
{
  "name": "mylib",
  "version": "1.0.0",
  "description": "My mType library",
  "source": "src",
  "library": "build/mylib.mtcLib",
  "dependencies": {
    "otherlib": "^1.0.0"
  }
}
```

| Field | Required | Meaning |
|---|---:|---|
| `name` | Yes | Package name. Also used for the `@name` import alias under `mt_modules/`. |
| `version` | Yes | Semantic version, for example `1.0.0`. |
| `description` | No | Human-readable package description. |
| `source` | No | Source directory inside the package. Defaults to `src`. |
| `library` | No | Optional compiled `.mtcLib` path. |
| `dependencies` | No | Map of package names to version ranges. |

### Git Package Flow

For a package hosted on GitHub:

1. Add `mtpkg.json` at the repo root.
2. Put package sources under the manifest's `source` directory, usually `src/`.
3. Commit the package.
4. Tag the version:

```bash
git tag v1.0.0
git push --tags
```

`mtpm` accepts either `v1.0.0` or `1.0.0` tags when fetching a git package.

Consumers can then add the package with:

```bash
mtpm add mylib@^1.0.0 --source github:owner/mylib
```

### Local Publish

`mtpm publish` copies the package described by `mtpkg.json` into the local registry:

```bash
mtpm publish
```

The default local registry is:

```text
~/.mtype/registry
```

Set `MTYPE_REGISTRY` to use another registry directory.

Useful publish options:

```bash
mtpm publish --force
mtpm publish --git-tag
```

- `--force` overwrites an existing local registry entry for the same package version.
- `--git-tag` creates a `v<version>` git tag, for example `v1.0.0`.

## Source Format

Sources can be:

- `github:user/repo` — shorthand for `https://github.com/user/repo.git`
- A full git URL: `https://...`, `git@...`
- A relative path for local development

## Roadmap

A central registry is on the roadmap. Until then, packages live wherever you can clone them with git.

## See Also

- [Projects & Workspaces](projects.md)
