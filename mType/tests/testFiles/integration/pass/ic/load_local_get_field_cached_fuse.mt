// MYT-198: LOAD_LOCAL + GET_FIELD_CACHED → LOAD_LOCAL_GET_FIELD_CACHED.
// Stable monomorphic field read from the same local receiver in a tight loop.
// After IC stabilization the pair fuses; output must match pre-fusion reads.

class Bag {
    public int lo;
    public int hi;
    public constructor() {
        this.lo = 3;
        this.hi = 11;
    }
}

Bag b = new Bag();
int sum = 0;
for (int i = 0; i < 1000; i = i + 1) {
    sum = sum + b.lo + b.hi;
}
print(sum);
