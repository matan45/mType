// MYT-378: an Object[16] (>= the size-16 SoA threshold) is the universal
// heap slot and must hold any class instance. Object has no fields, so the
// SoA columns can store nothing — the heterogeneous fallback preserves each
// element's real class and fields. Before the fix this threw on the first
// store with "ObjectInstance class mismatch".
print("Testing Object slot array");

class Repo {
    private string _id;
    public constructor(string v) {
        _id = v;
    }
    public function name(): string {
        return _id;
    }
}

class Animal {
    public function makeSound(): string {
        return "Woof";
    }
}

Object[] beans = new Object[16];
beans[0] = new Repo("alpha");
beans[1] = new Animal();
beans[2] = new Repo("beta");

Repo r0 = (Repo)beans[0];
print(r0.name());

Animal a1 = (Animal)beans[1];
print(a1.makeSound());

Repo r2 = (Repo)beans[2];
print(r2.name());

print("Done");
