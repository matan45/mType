// MYT-378: null interleaved with subtype elements in an Animal[16]. null is
// not the exact declared class, so it forces heterogeneous fallback; the
// `== null` guard and dispatch on non-null elements must both hold after the
// array has demoted from the SoA fast path.
print("Testing null interleaved array");

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

Animal[] zoo = new Animal[16];
for (int i = 0; i < 16; i++) {
    if (i % 2 == 0) {
        zoo[i] = new Dog();
    } else {
        zoo[i] = null;
    }
}

for (int i = 0; i < 16; i++) {
    if (zoo[i] == null) {
        print("null");
    } else {
        print(zoo[i].makeSound());
    }
}

print("Done");

// Expected output:
// Testing null interleaved array
// Woof
// null
// Woof
// null
// Woof
// null
// Woof
// null
// Woof
// null
// Woof
// null
// Woof
// null
// Woof
// null
// Done
