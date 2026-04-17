// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
// Exercises bitwise AND/OR/XOR and left/right shift in a tight loop.
// Baseline coverage for MYT-141 (JIT emission for bitwise ops).

int N = 10000000;
int acc = 1;
int mask = 255;
int xorKey = 165;
int wordMask = 65535;
for (int i = 0; i < N; i = i + 1) {
    int x = (i & mask) | (i >> 3);
    int y = (x ^ xorKey) << 1;
    int z = (y & wordMask) ^ (i << 2);
    acc = (acc ^ z) + (i & 1);
}

print("bitwise_tight_loop acc=" + acc);
