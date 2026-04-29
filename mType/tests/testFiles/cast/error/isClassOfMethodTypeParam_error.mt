// MYT-218: `isClassOf` against a method-level / free-function generic type
// parameter is rejected at compile time. Free generic functions don't carry
// reified type information at runtime, so `a isClassOf T` would silently
// return false. The compiler turns that into a clear diagnostic instead.
//
// EXPECTED: TypeException at compile time naming 'T' and the
// "always return false" reason at the source location of `a isClassOf T`.

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
