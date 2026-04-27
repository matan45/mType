// MYT-218: isClassOf against a generic type parameter is always false at runtime.
//
// EXPECTED (one of two):
//   1. Compile error: "isClassOf cannot test a generic type parameter (erased
//      at runtime)"
//   2. Reified-generic support: print "true" for the matching subtype
//
// ACTUAL (broken):
//   Compiles. Runs. The check `a isClassOf T` always returns false even when
//   the runtime type of `a` matches T. Output: "matches: false" — silently.

class Animal {
    protected string name;
    public constructor(string n) { this.name = n; }
    public function getName(): string { return this.name; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
}

class Inspector {
    public static function <T> isInstance(Animal a): bool {
        return a isClassOf T;
    }
}

Dog d = new Dog("Rex");
bool result = Inspector::isInstance<Dog>(d);
print("matches: " + result);
print("expected: true (Dog instance vs T=Dog)");
