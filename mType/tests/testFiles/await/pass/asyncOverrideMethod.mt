// Test override async methods

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Override Method Test ===");

class Animal {
    protected string name;

    public constructor(string n) {
        this.name = n;
    }

    public function async makeSound(): Promise<String> {
        print("Animal makeSound called for: " + this.name);
        return new String("Generic sound");
    }

    public function getName(): string {
        return this.name;
    }
}

class Dog extends Animal {
    public constructor(string n):super(n) {
    }

    public function async makeSound(): Promise<String> {
        print("Dog makeSound called for: " + super.name);
        return new String("Woof!");
    }
}

class Cat extends Animal {
    public constructor(string n):super(n) {
    }

    public function async makeSound(): Promise<String> {
        print("Cat makeSound called for: " + super.name);
        return new String("Meow!");
    }
}

function async testPolymorphism(Animal animal): Promise<String> {
    print("Testing animal: " + animal.getName());
    string sound = (await animal.makeSound()).getValue();
    print("Sound: " + sound);
    return new String(sound);
}

function async main(): Promise<String> {
    Dog dog = new Dog("Buddy");
    Cat cat = new Cat("Whiskers");
    Animal generic = new Animal("Unknown");

    string s1 = await testPolymorphism(dog);
    string s2 = await testPolymorphism(cat);
    string s3 = await testPolymorphism(generic);

    return new String(s3);
}

main();
