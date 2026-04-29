// MYT-228: `isClassOf` against a method-level / free-function generic type
// parameter is reified at runtime via per-call-frame typeArgBindings staged
// by BIND_TYPE_ARGS. Inspector::isInstance<Dog>(d) returns true when d is a
// Dog and false otherwise. Replaces the MYT-218 compile-time stopgap.

class Animal {
    protected string name;
    public constructor(string n) { this.name = n; }
    public function getName(): string { return this.name; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
}

class Cat extends Animal {
    public constructor(string n) : super(n) {}
}

class Inspector {
    public static function <T> isInstance(Animal a): bool {
        return a isClassOf T;
    }
}

Dog d = new Dog("Rex");
Cat c = new Cat("Whiskers");

print(Inspector::isInstance<Dog>(d));    // true
print(Inspector::isInstance<Dog>(c));    // false
print(Inspector::isInstance<Cat>(d));    // false
print(Inspector::isInstance<Cat>(c));    // true
print(Inspector::isInstance<Animal>(d)); // true (Dog is-a Animal)
print(Inspector::isInstance<Animal>(c)); // true (Cat is-a Animal)
