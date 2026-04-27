// MYT-215: a lambda created inside a loop may not capture a variable that's
// mutated within the loop. Capturing the for-loop counter `i` directly is the
// canonical footgun — every closure ends up seeing the post-loop value.
// The compiler now rejects this; the documented workaround is to copy `i`
// into a fresh local inside the body and capture that.

interface Function {
    function apply(int x): int;
}

function main(): void {
    Function[] funcs = new Function[3];

    for (int i = 0; i < 3; i = i + 1) {
        funcs[i] = x -> i + x;  // ERROR: 'i' is mutated within the enclosing loop
    }
}

main();
