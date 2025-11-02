// Test override async methods

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Override Method Test ===");

class Animal {
    string name;

    public constructor(string n) {
        this.name = n;
    }

    public function async makeSound(): Promise<string> {
        print("Animal makeSound called for: " + this.name);
        return "Generic sound";
    }

    public function getName(): string {
        return this.name;
    }
}

class Dog extends Animal {
    public constructor(string n) {
        super(n);
    }

    public function async makeSound(): Promise<string> {
        print("Dog makeSound called for: " + this.name);
        return "Woof!";
    }
}

class Cat extends Animal {
    public constructor(string n) {
        super(n);
    }

    public function async makeSound(): Promise<string> {
        print("Cat makeSound called for: " + this.name);
        return "Meow!";
    }
}

function async testPolymorphism(Animal animal): Promise<string> {
    print("Testing animal: " + animal.getName());
    string sound = await animal.makeSound();
    print("Sound: " + sound);
    return sound;
}

function async main(): Promise<string> {
    Dog dog = new Dog("Buddy");
    Cat cat = new Cat("Whiskers");
    Animal generic = new Animal("Unknown");

    string s1 = await testPolymorphism(dog);
    string s2 = await testPolymorphism(cat);
    string s3 = await testPolymorphism(generic);

    return s3;
}

main();
