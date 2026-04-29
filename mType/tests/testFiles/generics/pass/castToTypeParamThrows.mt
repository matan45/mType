// MYT-222: (T)cast to a generic type parameter throws at runtime.
//
// EXPECTED (one of two):
//   1. Erased semantics: (T)x is a no-op cast, always succeeds.
//      Output: "got Sub"
//   2. Reified semantics: (T)x checks the substituted type. When the runtime
//      type matches T, the cast succeeds. Output: "got Sub"
//
// ACTUAL (broken):
//   Runtime error: User exception thrown: Exception
//   The cast (T)x always throws regardless of x's actual type, even when
//   x is unambiguously a T (e.g. `this` inside a generic method on a class
//   that IS the type parameter's resolved type).

class Holder<T> {
    public function passThrough(T x): T {
        return (T)x;   // <-- throws at runtime
    }
}

class Box {
    public int v;
    public constructor(int v) { this.v = v; }
}

Holder<Box> h = new Holder<Box>();
Box b = new Box(42);
Box result = h.passThrough(b);
print("got " + result.v);
