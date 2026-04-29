// MYT-228: hot-loop benchmark for method-level / free-function generic
// dispatch. Each iteration goes through:
//   - BIND_TYPE_ARGS (stage T into the next frame's typeArgBindings)
//   - CALL_STATIC into a generic helper
//   - INSTANCEOF_TYPEPARAM (resolve T via TypeExecutor::resolveTypeParameter,
//     which walks the per-frame map first)
//
// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
//
// Acceptance: per-call overhead of the BIND_TYPE_ARGS + per-frame lookup
// path stays close to a non-generic call_method dispatch. Compare against
// method_dispatch.mt as a non-generic baseline. JIT must compile the
// loop body (jit_instanceof_typeparam helper) and not deopt on
// BIND_TYPE_ARGS (jit_bind_type_args is a pure helper-call emit).

class Animal {
    public constructor() {}
}

class Dog extends Animal {
    public constructor() : super() {}
}

class Cat extends Animal {
    public constructor() : super() {}
}

class Inspector {
    // Free generic-style static method. Each call site is monomorphic in T
    // at the source level (Inspector::isDog vs Inspector::isCat), so the
    // BIND_TYPE_ARGS payload is stable per site — exercises the IC-friendly
    // path.
    public static function <T> matches(Animal a): bool {
        return a isClassOf T;
    }
}

Animal[] zoo = new Animal[6];
zoo[0] = new Dog();
zoo[1] = new Cat();
zoo[2] = new Dog();
zoo[3] = new Dog();
zoo[4] = new Cat();
zoo[5] = new Dog();

int N = 1000000;
int dogHits = 0;
int catHits = 0;

for (int i = 0; i < N; i = i + 1) {
    Animal a = zoo[i % 6];
    if (Inspector::matches<Dog>(a)) {
        dogHits = dogHits + 1;
    }
    if (Inspector::matches<Cat>(a)) {
        catHits = catHits + 1;
    }
}

print("generic_dispatch_hot dogs=" + dogHits + " cats=" + catHits);
