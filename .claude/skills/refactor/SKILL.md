---
name: refactor
description: Code refactoring and cleanup discipline for mType. Use when asked to refactor, simplify, split oversized methods or classes, reorganize files by feature/domain, extract reusable utilities, reduce method parameters, remove redundant comments, remove MYT ticket comments, or remove dead code while preserving behavior.
---

# Refactor

Refactor in small, behavior-preserving steps. Read the surrounding code first, identify the current seams and tests, then make the smallest change that improves locality without changing public behavior.

## Limits

Use these limits as hard review gates for touched code:

- Methods must be 50 lines or shorter. Extract helper methods or split the workflow into named steps.
- `.cpp` files must be 500 lines or shorter. Split large implementations into focused feature/domain files.
- Methods must take no more than 5 parameters. Use a config or params struct for related inputs.
- Classes and implementation files must live in feature/domain folders. Do not add unrelated files to a flat shared dump.

## Utility Extraction

- Move standalone reusable helpers into dedicated utility classes or focused helper modules.
- Keep utility classes single-purpose. Do not create catch-all utility buckets.
- Update all call sites when moving helpers, and remove the old declarations/includes once unused.
- Prefer existing project naming, folder layout, and helper patterns before introducing a new utility location.

## Comment Cleanup

- Remove comments that only restate the code, narrate obvious control flow, or describe stale implementation history.
- Keep comments that explain non-obvious intent, invariants, ABI/runtime constraints, security constraints, or external compatibility.
- Remove `MYT-*` ticket references from comments in touched areas. Preserve the useful technical explanation by rewriting the comment without the ticket marker.
- Delete commented-out code instead of leaving it as documentation.

## Dead Code Cleanup

- Remove unused functions, classes, branches, local variables, declarations, includes, feature flags, and obsolete helpers only after confirming they are not referenced.
- Confirm dead code with search, compiler diagnostics, tests, and nearby architecture. Do not delete code solely because it looks unused.
- If code is public API, plugin-facing, reflection-facing, test-discovered, or dynamically referenced, keep it unless there is clear evidence it is dead.
- Remove corresponding tests only when they test deleted dead behavior; otherwise update tests to cover the surviving interface.

## Process

1. Inspect the relevant files, call sites, and tests before editing.
2. Define the behavior that must stay unchanged.
3. Apply one focused refactor at a time.
4. Update includes, declarations, build files, and call sites together.
5. Run the narrowest useful tests, then broaden if shared behavior changed.
6. Report any limit that remains violated and why it was left for a separate refactor.
