// MYT-228: forwarding case from ticket Risks #3.
// `inner<T>(...)` invoked from inside `outer<T>` must forward outer's T
// into inner's frame. The compiler tags the binding with valueKind=1
// (forward-from-caller); BIND_TYPE_ARGS resolves it against the caller
// frame at runtime so inner sees a concrete type, not the symbolic "T".

class Dog {
    public constructor() {}
}

class Cat {
    public constructor() {}
}

class Inspector {
    public static function <U> inner(Object x): bool {
        return x isClassOf U;
    }

    public static function <T> outer(Object x): bool {
        return Inspector::inner<T>(x);  // T forwarded from caller frame
    }
}

Dog d = new Dog();
Cat c = new Cat();

print(Inspector::outer<Dog>(d));  // true  — outer.T = Dog → inner.U = Dog → d isClassOf Dog
print(Inspector::outer<Dog>(c));  // false — outer.T = Dog → inner.U = Dog → c isClassOf Dog
print(Inspector::outer<Cat>(d));  // false
print(Inspector::outer<Cat>(c));  // true
