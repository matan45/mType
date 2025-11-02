import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test: Lower bound constraint violation
// Expected: Error - type parameter violates lower bound constraint

class Animal {
    string name;

    constructor(string n) {
        name = n;
    }

    public function getName(): string {
        return name;
    }

    public function describe(): string {
        return "Animal: " + name;
    }
}

class Dog extends Animal {
    constructor(string n) {
        super(n);
    }

    public function describe(): string {
        return "Dog: " + name;
    }

    public function bark(): string {
        return "Woof!";
    }
}

class Cat extends Animal {
    constructor(string n) {
        super(n);
    }

    public function describe(): string {
        return "Cat: " + name;
    }

    public function meow(): string {
        return "Meow!";
    }
}

class Plant {
    string species;

    constructor(string s) {
        species = s;
    }

    public function getSpecies(): string {
        return species;
    }
}

// Generic class with lower bound - accepts Dog or its supertypes
class PetContainer<T super Dog> {
    T pet;

    constructor(T p) {
        pet = p;
    }

    public function getPet(): T {
        return pet;
    }

    public function describe(): string {
        return pet.describe();
    }
}

function main(): void {
    Dog dog = new Dog("Buddy");
    PetContainer<Dog> dogContainer = new PetContainer<Dog>(dog);
    print(dogContainer.describe());

    Animal animal = new Animal("Generic");
    PetContainer<Animal> animalContainer = new PetContainer<Animal>(animal);
    print(animalContainer.describe());

    // ERROR: Cat is not a supertype of Dog
    // This violates the lower bound constraint T super Dog
    Cat cat = new Cat("Whiskers");
    PetContainer<Cat> catContainer = new PetContainer<Cat>(cat);
    print(catContainer.describe());
}

main();
