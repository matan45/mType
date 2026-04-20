// MYT-199: LOAD_LOCAL_BOXED_INST / STORE_LOCAL_BOXED_INST correctness.
// The `current` local holds a monomorphic ObjectInstance; its LOAD / STORE
// opcodes promote to the OBJECT variants. Output must match generic dispatch.

class Box {
    public int value;
    public constructor(int v) {
        this.value = v;
    }
}

function sumBoxes(Box b, int n): int {
    Box current = b;
    int sum = 0;
    for (int i = 0; i < n; i = i + 1) {
        sum = sum + current.value;
    }
    return sum;
}

Box b = new Box(7);
print(sumBoxes(b, 1000));
