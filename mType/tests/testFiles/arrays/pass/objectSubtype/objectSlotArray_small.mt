// MYT-378: a small Object[] (size 4, below the SoA threshold) is the universal
// slot and must hold any class instance. This is the ticket's literal repro
// (`Object[4] beans; beans[0] = new Repo()`), which threw
// "ObjectInstance class mismatch" before the fix.
print("Testing small Object slot array");

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

Object[] beans = new Object[4];
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

// Expected output:
// Testing small Object slot array
// alpha
// Woof
// beta
// Done
