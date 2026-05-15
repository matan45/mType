// MYT-316: hot-loop plain-function inlining. The ticket's acceptance
// criterion #1 is "tight loop calling a 3-instruction free function
// disassembles to no cc.invoke" — this is the correctness oracle for
// that scenario (we can't assert on disasm from .mt source, but if the
// inline path miscompiles the sum will not be 50000050000000 / 5000050000000).

function double(int x): int {
    return x * 2;
}

function triple(int x): int {
    return x * 3;
}

int sum = 0;
int n = 0;
while (n < 1000) {
    sum = sum + double(n) + triple(n);
    n = n + 1;
}
print(sum); // sum_{n=0..999} 5n = 5 * 999 * 1000 / 2 = 2497500
