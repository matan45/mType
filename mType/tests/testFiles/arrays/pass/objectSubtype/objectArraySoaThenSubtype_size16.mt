// MYT-378: fill an Animal[16] with the EXACT declared class first (indices
// 0-7 populate the SoA fast path), then store Dog/Cat subtypes at indices
// 8-15. The first subtype store triggers convertToHeterogeneous(), which must
// migrate the already-stored exact-class columns intact. All 16 elements must
// dispatch correctly afterwards.
print("Testing SoA-then-subtype conversion");

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

class Cat extends Animal {
    public function makeSound(): string {
        return "Meow";
    }
}

Animal[] zoo = new Animal[16];
// Indices 0-7: exact Animal (SoA fast path).
for (int i = 0; i < 8; i++) {
    zoo[i] = new Animal();
}
// Indices 8-15: subtypes (forces heterogeneous fallback mid-stream).
for (int i = 8; i < 16; i++) {
    if (i < 12) {
        zoo[i] = new Dog();
    } else {
        zoo[i] = new Cat();
    }
}

for (int i = 0; i < 16; i++) {
    print(zoo[i].makeSound());
}

print("Done");

// Expected output:
// Testing SoA-then-subtype conversion
// Generic
// Generic
// Generic
// Generic
// Generic
// Generic
// Generic
// Generic
// Woof
// Woof
// Woof
// Woof
// Meow
// Meow
// Meow
// Meow
// Done
