# mType 1.0.0 Known Limitations

The 1.0.0 release is a stable-core release. The following advanced gaps are documented follow-up work and do not block the release.

## Language Semantics

- Nested classes and nested interfaces are rejected at parse time (`mType/parser/ClassParser.cpp`, `mType/parser/InterfaceParser.cpp`).
- Generic types are invariant on cast: `Container<Dog>` is not assignable to `Container<Animal>` even when `Dog extends Animal`. Use explicit covariant / contravariant declarations where supported.
- `isClassOf`-style runtime type checks are conservative in deeply nested generic cases where runtime generic identity cannot be proven precisely.
- For-each loses the element type when the source expression's static type is `Object`. Affects nested `ArrayList<ArrayList<T>>`, multi-dimensional primitive arrays (`int[][]`), and ternary-result iterables. The compiler rejects with MT-E2007. Workaround: bind the source to a typed local before iterating.

## Standard Library

- `Stream.sorted()` is a stub that throws. `Comparable<T>` cannot be enforced at compile time on the stream's element type. Use `sortedWith(Comparator<T>)` instead (`lib/streams/StreamImpl.mt`).
- `HashMap.containsValue` returns false for value-class entries. The hash-gated dispatch used by `put`/`get`/`remove` is not applied here, so generic `V.equals(value)` falls back to reference equality instead of resolving to the value-class `equals` override. Workaround: scan via `values().iterator()` and call `equals` manually.

## Runtime and JIT

- JIT specialization is limited to proven hot paths and falls back to interpreter execution for unsupported opcodes.
- Multi-dimensional array sub-rows are views, not owning arrays: `arr[i] = newRow` raises MT-E5005. The single-entry sub-array view cache in `FlatMultiArray` is intentionally narrow.
- Shape-specialized generics have eligibility limits — polymorphic inline-cache entries must all clear the per-entry checks, the combined instruction count must stay within the inline-size budget, and callees that use custom equality / hash dispatch may be excluded.
- Depth-2 OSR inlining can hang on certain stream pipelines. When both an outer `count()`-style loop and an inner iterator's `advance()` loop become hot enough to OSR-compile (roughly when the inner sequence has ~6 or more elements), the depth-2 inlined view of `hasNextElement` can be read as permanently true and the pipeline never terminates. Function-level JIT and depth-1 inlining are unaffected. Workaround: lower `INLINE_DEPTH_LIMIT` to 0 or 1 in `mType/vm/optimization/InlineAnalysis.hpp` to disable depth-2 nested inlining.

## Language Server

- Range formatting is not implemented. The server advertises full-document formatting only (`documentRangeFormattingProvider: false` in `languageserver/src/MTypeLanguageServer.cpp`); `formatRange()` currently delegates to `formatDocument()`.

## Packaging

- GitHub Releases are the only 1.0.0 distribution channel. Prebuilt archives for Windows, Linux, and macOS are published per tag.
- Marketplace publishing (VS Code), MSI, Homebrew, apt, winget, Chocolatey, and Scoop packages are out of scope for 1.0.0.
