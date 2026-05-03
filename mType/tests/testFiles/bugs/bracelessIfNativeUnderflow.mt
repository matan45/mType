// MYT-271: braceless `if (!fn()) <single-stmt>;` produces invalid bytecode and
// crashes with MT-E5005 ("Stack underflow: required 1 values but stack has 0")
// when fn() calls a native (e.g. hashCode, parsePrimitive, strLength).
// Reproduces under both --no-jit and JIT, so the bug is in the AST -> bytecode
// compiler, not the JIT.
//
// All four conditions are required to trigger:
//   1. Braceless if-body (single statement with no { }).
//   2. The condition uses the `!` operator.
//   3. The condition is a user-function call.
//   4. The user function's body calls a native.
// Removing any one makes it run correctly.
//
// EXPECTED:
//   1
//
// ACTUAL (broken):
//   error[MT-E5005]: Stack underflow: required 1 values but stack has 0

function check(int v): bool {
    int h = hashCode(v);
    return h > 0;
}

function main(): void {
    int fails = 0;
    if (!check(42)) fails = fails + 1;
    if (!check(0)) fails = fails + 1;
    print(fails);
}
main();
