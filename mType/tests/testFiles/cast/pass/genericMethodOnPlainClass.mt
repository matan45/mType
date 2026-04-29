// MYT-228: generic <T> method on a NON-generic class. The receiver
// has no class-level bindings, so resolveTypeParameter must hit the
// per-frame typeArgBindings path exclusively (the receiver-bindings
// fall-back returns nothing).

class Tagger {
    public string label;
    public constructor(string l) { this.label = l; }

    public function <T> matches(Object o): bool {
        return o isClassOf T;
    }
}

class Apple {
    public constructor() {}
}

class Orange {
    public constructor() {}
}

Tagger t = new Tagger("fruit-checker");
Apple a = new Apple();
Orange o = new Orange();

print(t.label);
print(t.matches<Apple>(a));   // true
print(t.matches<Apple>(o));   // false
print(t.matches<Orange>(a));  // false
print(t.matches<Orange>(o));  // true
