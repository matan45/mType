// TARGET: ~2s on dev machine. Adjust iterations if first run lands outside 1-5s.
// Exercises virtual method dispatch through a polymorphic call site.
// shapes[i % 3].compute(i) is one call site seeing three runtime classes -
// monomorphic inline caches should miss, polymorphic IC should hit.

class Shape {
    public function compute(int x): int {
        return x;
    }
}

class A extends Shape {
    @Override
    public function compute(int x): int {
        return x * 2;
    }
}

class B extends Shape {
    @Override
    public function compute(int x): int {
        return x + 3;
    }
}

class C extends Shape {
    @Override
    public function compute(int x): int {
        return x - 1;
    }
}

Shape[] shapes = [new A(), new B(), new C()];

int iterations = 2000000;
int acc = 0;
for (int i = 0; i < iterations; i = i + 1) {
    acc = acc + shapes[i % 3].compute(i);
}

print("method_dispatch acc=" + acc);
