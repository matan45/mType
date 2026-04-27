// Pinned behavior: lambda captures the mutable for-loop counter. mType allows
// it; this test documents the observed semantics (by-value vs by-reference).

interface Function {
    function apply(int x): int;
}

function main(): void {
    print("=== Lambda Capture Mutable Loop Var ===");

    Function[] funcs = new Function[3];

    for (int i = 0; i < 3; i = i + 1) {
        funcs[i] = x -> i + x;
    }

    print("funcs[0](10) = " + funcs[0].apply(10));
    print("funcs[1](10) = " + funcs[1].apply(10));
    print("funcs[2](10) = " + funcs[2].apply(10));
}

main();
