// MYT-228: (T)cast through a free generic, where T resolves to different
// classes in a small hierarchy. Verifies that BIND_TYPE_ARGS-staged
// bindings apply to CAST_TYPEPARAM (not just INSTANCEOF_TYPEPARAM) and
// that the runtime cast machinery picks the right concrete class.

class Animal {
    public string kind;
    public constructor(string k) { this.kind = k; }
}

class Dog extends Animal {
    public int barkLevel;
    public constructor(int b) : super("dog") { this.barkLevel = b; }
}

class Puppy extends Dog {
    public constructor() : super(3) {}
}

class Caster {
    public static function <T> as(Animal a): T {
        return (T)a;
    }
}

Puppy p = new Puppy();
Animal a = p;

// Cast back to each level of the hierarchy. Each call sets a different
// concrete T on the next frame.
Animal asA = Caster::as<Animal>(a);
Dog asD = Caster::as<Dog>(a);
Puppy asP = Caster::as<Puppy>(a);

print(asA.kind);
print(asD.kind);
print(asD.barkLevel);
print(asP.kind);
print(asP.barkLevel);
