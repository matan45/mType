// MYT-226: pure tail self-recursion must throw "Stack overflow:" via the
// JIT's per-frame depth counter, mirroring the interpreter's pushCallFrame
// guard. Companion to runtimeStackOverflow.mt (which uses + 1 to disable TCO).

function recurse(int depth): int {
    return recurse(depth + 1);
}
int result = recurse(0);
print(result);
