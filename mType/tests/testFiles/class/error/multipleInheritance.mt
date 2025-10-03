// Test: Multiple inheritance - Class extends multiple parents
// Expected: Error - multiple inheritance not supported (single inheritance only)

class Animal {
    String species;

    constructor(String species) {
        this.species = species;
    }

    function move(): void {
        print("Animal moves");
    }
}

class Pet {
    String owner;

    constructor(String owner) {
        this.owner = owner;
    }

    function play(): void {
        print("Pet plays");
    }
}

// ERROR: Cannot extend multiple classes - mType supports single inheritance only
class Dog extends Animal, Pet {
    String name;

    constructor(String name, String species, String owner) {
        super(species);
        this.name = name;
        // How would we call Pet's constructor? This is why multiple inheritance is problematic
    }
}

// This should never execute due to multiple inheritance error
Dog myDog = new Dog("Buddy", "Canis familiaris", "John");
