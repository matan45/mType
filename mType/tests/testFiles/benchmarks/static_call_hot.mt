// Isolates non-generic CALL_STATIC dispatch. Companion to generic_dispatch_hot.mt
// (which mixes BIND_TYPE_ARGS + CALL_STATIC + INSTANCEOF_TYPEPARAM); this one
// strips the generics so the measured cost is purely the static-call IC path.
//
// Hot loop calls 4 distinct static methods per iteration. Each call site is
// trivially monomorphic (the qualified name is baked into the bytecode operand),
// so a single-entry IC keyed by call-site IP collapses every per-call hashmap
// probe into a cached pointer load.
//
// TARGET: ~1-2s on dev machine. Adjust N if first run lands outside 1-5s.
//
// Compare against method_dispatch.mt (~125ms) — that's the floor for
// non-allocating polymorphic dispatch with IC; static dispatch should land
// at or below it.

class Math {
    public static function inc(int x): int {
        return x + 1;
    }

    public static function double_it(int x): int {
        return x * 2;
    }

    public static function clamp(int x, int lo, int hi): int {
        if (x < lo) return lo;
        if (x > hi) return hi;
        return x;
    }

    public static function isPositive(int x): bool {
        return x > 0;
    }
}

int N = 2000000;
int acc = 0;
int posHits = 0;

for (int i = 0; i < N; i = i + 1) {
    int v = Math::inc(i);
    v = Math::double_it(v);
    v = Math::clamp(v, 0, 1000);
    if (Math::isPositive(v)) {
        posHits = posHits + 1;
    }
    acc = acc + v;
}

print("static_call_hot acc=" + acc + " pos=" + posHits);
