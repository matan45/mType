# Bug Repros (MYT-211 family)

These `.mt` files are minimal reproductions of the language bugs filed under
parent story **MYT-211** ("Language bugs surfaced by edge-case test authoring").

Each file maps to one subtask:

| Subtask  | File                                       | Summary                                                     |
|----------|--------------------------------------------|-------------------------------------------------------------|
| MYT-212  | ~~`superFieldDynamicDispatch.mt`~~         | ~~`super.x` reads child's shadowed field~~ — fixed; promoted to `class/pass/superFieldStaticBinding.mt` |
| MYT-213  | ~~`getMethodsNoInheritance.mt`~~           | ~~`getMethods()` doesn't walk inheritance~~ — fixed; covered by `reflection/pass/reflectInheritedMethods.mt` |
| MYT-214  | ~~`getParameterCountIncludesThis.mt`~~     | ~~`getParameterCount()` includes `this`~~ — fixed; promoted to `reflection/pass/getParameterCountExcludesThis.mt` |
| MYT-215  | ~~`lambdaMutableLoopCapture.mt`~~          | ~~Lambda captures mutable for-loop var by reference~~ — fixed; promoted to `lambda/error/lambdaCaptureMutableLoopVar.mt` |
| MYT-216  | `threeTypeArgDropsArg.mt`                  | 3-type-param call drops trailing positional arg~~ — fixed; promoted to `generics/pass/threeTypeArgDropsArg.mt` |
| MYT-217  | ~~`genericNullableReturnSubstitution.mt`~~ | ~~Generic `T?` return doesn't substitute on assignment~~ — fixed; promoted to `generics/pass/genericNullableReturnSubstitution.mt` |
| MYT-218  | ~~`isClassOfTypeParamAlwaysFalse.mt`~~     | ~~`isClassOf T` against type-param silently always false~~ — fixed; promoted to `cast/error/isClassOfMethodTypeParam_error.mt` |
| MYT-219  | `switchBreakAfterReturnVoid.mt`            | `case x: return ...; break;` infers void return             |
| MYT-220  | `nullableCastSyntaxRejected.mt`            | `(T?)null` cast syntax rejected by parser                   |
| MYT-221  | `libFunctionMultiMethod.mt`                | `lib/functional/Function` is not single-method (lambda-incompatible) |
| MYT-222  | `castToTypeParamThrows.mt`                 | `(T)cast` to a generic type parameter throws at runtime     |
| MYT-223  | ~~`staticOverloadReturnsVoid.mt`~~         | ~~Static method overload call types as `void` at call site~~ — fixed; promoted to `overloading/pass/staticOverloadReturnsVoid.mt` |
| MYT-224  | `genericOverloadFunctionalInterfaceUnify.mt` | Overload resolution doesn't unify concrete `UnaryFn<Int,Int>` with `UnaryFn<T,R>` |
| MYT-225  | `getFieldTypedNonThisAccess.mt`            | `paramName.field` / `localVar.field` on class/value-class throws GET_FIELD_TYPED |
| MYT-226  | `jitTailCallNoOverflow.mt`                 | JIT TCO bypasses pushCallFrame's stack-overflow guard — infinite tail recursion hangs |
| MYT-227  | `infiniteLoopHangs.mt`                     | for/while loop back-edge has no interrupt poll — `while (true) {}` hangs with no diagnostic |

## Running

These tests are intentionally NOT registered in any `*TestSuite.cpp`.
They are known-failing until the underlying bug is fixed. Run individually:

```
bin\mType\Release\x64\mType.exe mType/tests/testFiles/bugs/<filename>.mt
```

## When a bug is fixed

1. Update the test so it asserts the now-correct behavior (replace the
   "ACTUAL (broken)" comment with the expected output).
2. Move the file from `bugs/` to the appropriate `<feature>/pass/` (or
   `<feature>/error/`) folder.
3. Add a paired `.expected` file if it's a pass test.
4. Register it in the matching `<Feature>TestSuite.cpp`.
5. Resolve the corresponding subtask under MYT-211.
