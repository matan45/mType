// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises NEW_ARRAY allocation inside a hot loop body — an array literal is
// built fresh on every iteration. Pairs with array_multi_alloc (multi-dim
// allocation, low iteration count) — this one isolates the single-dim
// allocation overhead at high iteration count.

int N = 500000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    int[] arr = [i, i + 1, i + 2, i + 3, i + 4];
    total = total + arr[0] + arr[2] + arr[4];
}
print("array_literal_alloc_hot total=" + total);
