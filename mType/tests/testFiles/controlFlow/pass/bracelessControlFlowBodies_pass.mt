// MYT-271: braceless if/while/do-while/for bodies leaked operand-stack
// slots when the callee body invoked a native (defeating constant
// folding / inlining), causing MT-E5005 stack underflow at runtime
// under both JIT and --no-jit. Covers the four broken sites in
// ControlFlowCompiler:
//   - compileIf:      then-branch and else-branch braceless single stmts
//   - compileWhile:   braceless body
//   - compileDoWhile: braceless body
//   - compileFor:     braceless body
// And both cleanup paths inside StatementCleanup:
//   (A) bare expression statements that are FunctionCall/MethodCall
//   (B) bare assignment statements (STORE_LOCAL re-pushes the stored
//       value to support cascading expressions like int x = (a = 5))

// Native call inside the body — defeats inlining/folding so the buggy
// AST shape reaches bytecode emission unchanged. hashCode applies
// `& 0x7FFFFFFF` so it always returns a non-negative int, giving us
// stable always-true / always-false predicates.
function isTrue(int v): bool {
    int h = hashCode(v);
    return h >= 0;          // always true
}

function isFalse(int v): bool {
    int h = hashCode(v);
    return h < 0;           // always false
}

function side(): void {
    print("se");
}

function main(): void {
    int counter = 0;

    // --- compileIf then-branch, Path A (function-call body) ---
    if (isTrue(1)) side();

    // --- compileIf then-branch, Path B (assignment body) ---
    if (isTrue(2)) counter = counter + 1;

    // --- compileIf else-branch, Path A ---
    if (isFalse(3)) side(); else side();

    // --- compileIf else-branch, Path B ---
    if (isFalse(4)) counter = counter + 100; else counter = counter + 10;

    // --- compileWhile body, Path B (braceless assignment) ---
    int wi = 0;
    while (wi < 3) wi = wi + 1;
    print(wi);

    // --- compileDoWhile body, Path B (braceless assignment) ---
    int dk = 0;
    do dk = dk + 1; while (dk < 2);
    print(dk);

    // --- compileFor body, Path B (braceless assignment) ---
    int fc = 0;
    for (int i = 0; i < 3; i = i + 1) fc = fc + 1;
    print(fc);

    print(counter);
    print("DONE");
}
main();
