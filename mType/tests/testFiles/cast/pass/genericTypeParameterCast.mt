// Test: (T)expr inside a generic class — exercises CAST_TYPEPARAM, which
// resolves T at runtime via the receiver's type-argument bindings.
class Animal {
    public string label;
    public constructor(string l) { this.label = l; }
}

class Dog extends Animal {
    public constructor(string l) : super(l) {}
    public function bark(): string { return "woof from " + this.label; }
}

class Caster<T> {
    public function cast(Animal a): T {
        return (T)a;
    }
}

Caster<Dog> c = new Caster<Dog>();
Animal upcasted = new Dog("Rex");
Dog d = c.cast(upcasted);
print(d.bark());

// Expected output:
// woof from Rex
