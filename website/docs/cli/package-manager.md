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

## Source Format

Sources can be:

- `github:user/repo` — shorthand for `https://github.com/user/repo.git`
- A full git URL: `https://...`, `git@...`
- A relative path for local development

## Roadmap

A central registry is on the roadmap. Until then, packages live wherever you can clone them with git.

## See Also

- [Projects & Workspaces](projects.md)
