// MYT-194: GET_FIELD_CACHED correctness under a stable monomorphic site.
// Runs enough iterations that the field IC stabilizes to MONO and the
// promoter rewrites GET_FIELD -> GET_FIELD_CACHED. The GET_FIELD inside
// readValue (top-level function — not a trivial-getter method, so no
// INLINE_GET_FIELD rewrite) is the IP that gets promoted. Output must
// match the pre-rewrite generic GET_FIELD dispatch.

class Counter {
    public int value;
    public constructor() {
        this.value = 7;
    }
}

function readValue(Counter c): int {
    return c.value;
}

Counter c = new Counter();
int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    acc = acc + readValue(c);
}
print(acc);
