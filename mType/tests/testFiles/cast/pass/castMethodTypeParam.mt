// MYT-228: (T)cast inside a free generic function dispatches via the
// per-frame typeArgBindings. Pre-MYT-228 the cast erased to a no-op
// (CAST_TYPEPARAM swallow path) because resolveTypeParameter couldn't
// find a binding for free generics. After MYT-228, BIND_TYPE_ARGS stages
// the concrete type into the frame and the cast routes through the
// proper class-cast machinery.

class Animal {
    public string name;
    public constructor(string n) { this.name = n; }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}
    public function bark(): string { return "woof"; }
}

class Caster {
    // Free generic function-style helper on a static method. Cast the
    // animal arg to T; calling .name on the result still works because
    // T is constrained to be Animal-compatible at the call site.
    public static function <T> castAs(Animal a): T {
        return (T)a;
    }
}

Dog d = new Dog("Rex");
Animal a = d;  // upcast for the helper

// Cast back to Dog via method-level T = Dog. After MYT-228, the cast
// resolves to "Dog" and the runtime cast machinery succeeds.
Dog back = Caster::castAs<Dog>(a);
print(back.name);
print(back.bark());
