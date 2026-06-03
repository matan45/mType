// MYT-378: an Animal[16] (>= the size-16 SoA threshold) must hold Dog/Cat
// subtypes, with polymorphic dispatch surviving the SoA -> heterogeneous
// fallback. Before the fix this threw "ObjectInstance class mismatch".
print("Testing subtype object array");

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

Animal[] zoo = new Animal[16];
for (int i = 0; i < 16; i++) {
    if (i < 8) {
        zoo[i] = new Dog();
    } else {
        zoo[i] = new Cat();
    }
}

for (int i = 0; i < 16; i++) {
    print(zoo[i].makeSound());
}

print("Done");
