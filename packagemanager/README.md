# mtpm — mType Package Manager

`mtpm` is the standalone CLI that resolves, fetches, and installs mType package dependencies declared in a `.mtproj` file. It produces a populated `mt_modules/` directory and a `mtproj.lock` lockfile next to the project.

## Build

```
cd packagemanager
runPremake.bat
```

Open `mtpm.sln` in Visual Studio (MSVC v145, C++20) and build. Output binary:

```
packagemanager/bin/{Debug|Release}-windows-x86_64/mtpm.exe
```

On Linux/macOS, generate makefiles with `premake5 gmake2` and `make`.

## Commands

```
mtpm install [--force-resolve]                          # install all deps from .mtproj
mtpm add <name>@<version> [--source github:user/repo]   # add and install one package
mtpm remove <name>                                      # remove a package
mtpm list                                               # show installed packages
mtpm init <name> <version>                              # scaffold mtpkg.json
mtpm publish [--force] [--git-tag] [dir]                # publish current package to the local registry
mtpm --help | --version
```

All commands accept an optional trailing `.mtproj` path; otherwise the nearest `.mtproj` in the current directory is used.

Version specs accept exact versions (`1.2.3`), caret (`^1.2.0` ≡ `>=1.2.0 <2.0.0`), tilde (`~1.2.0` ≡ `>=1.2.0 <1.3.0`), and explicit ranges (`>=1.0.0 <2.0.0`). Pre-release suffixes (`1.0.0-alpha`) are not supported.

The default registry lives at `~/.mtype/registry/`. Override with the `MTYPE_REGISTRY` environment variable.

## Project declaration

Dependencies are declared inside the `<Dependencies>` block of an `.mtproj`:

```xml
<Project Name="MyProject" Version="1.0.0">
    <Dependencies>
        <Package Name="mathlib" Version="^1.0.0" />
        <Package Name="utils" Version="~2.1.0" Source="github:someone/utils" />
    </Dependencies>
</Project>
```

## Package manifest — `mtpkg.json`

```json
{
    "name": "mathlib",
    "version": "1.2.0",
    "description": "Math utilities",
    "source": "src",
    "library": "build/mathlib.mtcLib",
    "dependencies": {
        "utils": "^2.0.0"
    }
}
```

`name` and `version` are required; everything else is optional. `source` (default `"src"`) is the subdirectory consumers will import from.

## Lockfile — `mtproj.lock`

Written alongside the `.mtproj` after every install. Format:

```json
{
    "version": 1,
    "packages": {
        "mathlib": {
            "version": "1.2.0",
            "resolved": "file:///.../registry/mathlib/1.2.0",
            "integrity": "sha256-<hex>",
            "dependencies": { "utils": "^2.0.0" }
        }
    }
}
```

A fresh lockfile is reused on subsequent installs without re-resolving the dependency tree. SHA-256 integrity hashes are verified against the on-disk registry contents on every install — a mismatch aborts the install with `"Integrity mismatch for <name>@<version>"`.

## Registry layout

```
~/.mtype/registry/
    mathlib/
        1.0.0/mtpkg.json
        1.2.0/mtpkg.json
        2.0.0/mtpkg.json
    utils/
        2.0.0/mtpkg.json
```

Remote sources are git-based. `github:user/repo` expands to `https://github.com/user/repo.git`; full git URLs also work. Fetching uses `git ls-remote --tags` followed by a shallow clone of the matching tag. Tags `vX.Y.Z` and `X.Y.Z` are both accepted.

## Source layout

```
packagemanager/
├── premake5.lua              # build spec — globs src/**.{hpp,cpp}
├── runPremake.bat            # premake5 vs2022
├── mtpm.sln / mtpm.vcxproj   # generated
└── src/
    ├── Main.cpp              # CLI dispatch
    ├── SemVer.{hpp,cpp}      # version parsing and range matching
    ├── PackageManifest.*     # mtpkg.json read/write
    ├── Lockfile.*            # mtproj.lock read/write
    ├── PackageRegistry.*     # local registry + git-source registration
    ├── GitSource.*           # git ls-remote / clone, path safety
    ├── DependencyResolver.*  # recursive resolution, circular detection
    ├── PackageInstaller.*    # install pipeline, lockfile handling
    ├── MtModulesManager.*    # mt_modules/ scanning, alias map
    └── Sha256.*              # directory integrity hashing
```

The build pulls in a handful of shared mType sources (`ProjectConfigParser`, `XmlParser`, `JsonParser`, `FileReader`) so `mtpm` and the main interpreter stay in sync on file formats.

## Testing

There is no test runner inside `mtpm` itself. The library code is exercised by `PackageManagerTestSuite` at `../mType/tests/suites/PackageManagerTestSuite.cpp`, run via the main `mType.exe` test runner. Fixtures live at `../mType/tests/testFiles/packagemanager/{project,registry}/` and cover SemVer, manifest round-trips, lockfile serialization, SHA-256 hashing, dependency resolution (including circular detection), and the installer. The `mtpm` CLI argument parsing in `Main.cpp` is not currently covered end-to-end.

## VS Code integration

The `mtype-language-support` extension exposes the package manager as commands and a tree view:

- `mType: Install Packages` / `Add Package…` / `Remove Package…` / `List Packages` (command palette and explorer context menu on `.mtproj` files)
- **mType Modules** view in the explorer, populated from `mt_modules/` and refreshed after every command

The extension shells out to the `mtpm` binary located via `mType.packageManager.path` (or the default path next to the extension).

## Import resolution

After `mtpm install`, the `ImportManager` automatically picks up `mt_modules/` — `import @mathlib/foo` resolves to `mt_modules/@mathlib/foo.mt` without any `<Alias>` declaration in the `.mtproj`. Aliases remain available for opt-in renames.
