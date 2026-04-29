// MYT-228: type-arg inference (Tier 2 — from arguments) followed by
// reified runtime dispatch. The caller doesn't write <T> explicitly;
// FunctionCallHelper::setupGenericTypeBindings infers T from the
// `sample` argument, then BIND_TYPE_ARGS stages the inferred concrete
// name into the next frame.

class Animal {
    public string name;
    public constructor(string n) { this.name = n; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
}

class Cat extends Animal {
    public constructor(string n) : super(n) {}
}

// T is inferred from the type of `sample`.
function <T> isSameKind(T sample, Object o): bool {
    return o isClassOf T;
}

Dog d1 = new Dog("Rex");
Dog d2 = new Dog("Buddy");
Cat c = new Cat("Whiskers");

// T inferred = Dog
print(isSameKind(d1, d2));  // true
print(isSameKind(d1, c));   // false

// T inferred = Cat
print(isSameKind(c, d1));   // false
print(isSameKind(c, c));    // true
