// MYT-378: a 3-level inheritance chain (Animal -> Dog -> Puppy). Storing the
// grandchild Puppy into an Animal[16] must succeed and dispatch the most-
// derived override. The exact-class check is depth-agnostic: any non-exact
// class (including a grandchild) falls back to heterogeneous storage.
print("Testing deep inheritance array");

class Animal {
    public function makeSound(): string {
        return "Generic";
    }
}

class Dog extends Animal {
    public function makeSound(): string {
        return "Woof";
    }
}

class Puppy extends Dog {
    public function makeSound(): string {
        return "Yip";
    }
}

Animal[] zoo = new Animal[16];
for (int i = 0; i < 16; i++) {
    zoo[i] = new Puppy();
}

for (int i = 0; i < 16; i++) {
    print(zoo[i].makeSound());
}

print("Done");

// Expected output:
// Testing deep inheritance array
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Yip
// Done
