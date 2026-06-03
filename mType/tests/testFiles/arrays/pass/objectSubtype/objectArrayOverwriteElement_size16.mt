// MYT-378: after an Animal[16] has fallen back to heterogeneous storage,
// overwriting an element (set() on an already-converted array) must work for
// any class — another subtype, or the exact declared class. Read back each
// overwrite to confirm the slot updates.
print("Testing element overwrite");

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
for (int i = 0; i < 16; i++) {
    zoo[i] = new Dog();
}

print(zoo[5].makeSound());
zoo[5] = new Cat();
print(zoo[5].makeSound());
zoo[5] = new Animal();
print(zoo[5].makeSound());

print("Done");

// Expected output:
// Testing element overwrite
// Woof
// Meow
// Generic
// Done
