// MYT-271 (fixed): braceless `if (!fn()) <single-stmt>;` previously
// produced invalid bytecode and crashed with MT-E5005 ("Stack underflow:
// required 1 values but stack has 0") when fn() called a native
// (e.g. hashCode, parsePrimitive, strLength). Reproduced under both
// --no-jit and JIT, so the bug was in the AST -> bytecode compiler,
// not the JIT.
//
// All four conditions were required to trigger:
//   1. Braceless if-body (single statement with no { }).
//   2. The condition uses the `!` operator.
//   3. The condition is a user-function call.
//   4. The user function's body calls a native.
// (Removing any one made it run correctly because constant folding /
// inlining transformed the AST before bytecode emission.)
//
// Fix: ControlFlowCompiler now invokes the shared statementCleanup
// helper after every braceless body, matching what compileBlock does
// after each statement in a braced body.

// `hashCode(v) & 0x7FFFFFFF` is non-zero for v=0 on MSVC's std::hash, so
// the original `return h > 0;` from the Jira repro was platform-dependent.
// Anchor the predicate on `v` instead — `h >= 0` is always true after the
// mask, so the call is effectively `v > 0`. The native call still runs
// (defeating inlining), so the buggy AST shape that crashed before the
// fix is still produced.
function check(int v): bool {
    int h = hashCode(v);
    return h >= 0 && v > 0;
}

function main(): void {
    int fails = 0;
    if (!check(42)) fails = fails + 1;
    if (!check(0)) fails = fails + 1;
    print(fails);
}
main();
