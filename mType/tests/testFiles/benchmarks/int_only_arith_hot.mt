// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Pure integer-arithmetic microbenchmark — no floats, no string ops. Pairs
// with arithmetic_tight_loop.mt (which mixes int and float) to isolate INT
// register pressure and the type-quickened LOAD_LOCAL_INT / arithmetic INT
// opcode wins.

int N = 5000000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    int t = (i * 3) - (i / 2) + (i & 7);
    acc = acc + t;
}
print("int_only_arith_hot acc=" + acc);
