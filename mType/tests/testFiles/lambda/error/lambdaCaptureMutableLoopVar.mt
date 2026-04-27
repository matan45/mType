// ERROR: Lambda inside a for-loop captures the mutable loop counter directly
// without it being declared final or made effectively-final via a copy.

interface Function {
    function apply(int x): int;
}

function main(): void {
    Function[] funcs = new Function[3];

    for (int i = 0; i < 3; i = i + 1) {
        // ERROR: 'i' is the mutable loop counter and is not effectively final
        funcs[i] = x -> i + x;
    }

    print("This should not execute");
}

main();
