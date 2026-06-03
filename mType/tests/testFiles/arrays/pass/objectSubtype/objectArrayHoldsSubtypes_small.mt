// MYT-378: a small object array (size 4, below the size-16 SoA threshold) uses
// the plain Value[] path and must hold subtypes. Mirrors the ticket's literal
// repro (`new Animal[1]`), which threw "ObjectInstance class mismatch" before
// the fix.
print("Testing small subtype array");

class Animal {
    public function makeSound(): string {
        return "...";
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

Animal[] pets = new Animal[4];
pets[0] = new Dog();
pets[1] = new Cat();
pets[2] = new Dog();
pets[3] = new Cat();

for (int i = 0; i < 4; i++) {
    print(pets[i].makeSound());
}

print("Done");

// Expected output:
// Testing small subtype array
// Woof
// Meow
// Woof
// Meow
// Done
