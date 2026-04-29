// MYT-228: hot-loop test exercising the JIT path for INSTANCEOF_TYPEPARAM
// with method-level T. Many iterations of the same generic call site let
// the inline cache stabilise and the JIT compile the body, which routes
// through jit_instanceof_typeparam (the new resolver-aware helper).
// Counts must match the interpreter result exactly — a JIT/interpreter
// divergence would surface as wrong totals.

import * from "../../lib/primitives/Int.mt";

class Animal {
    public constructor() {}
}

class Dog extends Animal {
    public constructor() : super() {}
}

class Cat extends Animal {
    public constructor() : super() {}
}

class Counter {
    public static function <T> count(Object[] xs): int {
        int n = 0;
        for (int i = 0; i < xs.length; i = i + 1) {
            if (xs[i] isClassOf T) {
                n = n + 1;
            }
        }
        return n;
    }
}

// 6 elements: 4 Dogs, 2 Cats. Looped 200 times to exercise warm-up.
Animal[] zoo = new Animal[6];
zoo[0] = new Dog();
zoo[1] = new Cat();
zoo[2] = new Dog();
zoo[3] = new Dog();
zoo[4] = new Cat();
zoo[5] = new Dog();

int totalDogs = 0;
int totalCats = 0;
for (int rep = 0; rep < 200; rep = rep + 1) {
    totalDogs = totalDogs + Counter::count<Dog>(zoo);
    totalCats = totalCats + Counter::count<Cat>(zoo);
}

print(totalDogs);   // 4 * 200 = 800
print(totalCats);   // 2 * 200 = 400
