// Tier B: array-literal assignment validates each element against the
// declared element type. Inserting a non-subclass element should error.
// Targets: StatementCompiler.cpp:898-914 (array literal element loop).
import * from "../../lib/primitives/String.mt";

class Animal {
    public constructor() {}
}

class Dog extends Animal {
    public constructor() {}
}

// Dog inserted alongside an unrelated String. Animal[] must reject String.
Animal[] arr = { new Dog(), new String("not an animal") };

print("should not reach");
