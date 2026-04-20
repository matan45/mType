// MYT-194: SET_FIELD_CACHED correctness under a stable monomorphic site.
// Mirrors get_field_cached_mono.mt for SET. Top-level writeValue hosts the
// SET_FIELD that gets promoted to SET_FIELD_CACHED once MONO. Reading the
// final value after the loop must match the last written value.

class Box {
    public int x;
    public constructor() {
        this.x = 0;
    }
}

function writeValue(Box b, int v): void {
    b.x = v;
}

Box b = new Box();
for (int i = 0; i < 1000; i = i + 1) {
    writeValue(b, i);
}
print(b.x);
