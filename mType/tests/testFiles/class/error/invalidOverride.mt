// Test: Invalid override - Method override with wrong signature
// Expected: Error - override signature doesn't match parent method

class Animal {
    String name;

    constructor(String name) {
        this.name = name;
    }

    function makeSound(int volume): void {
        print("Animal makes sound at volume " + volume);
    }

    function getInfo(): String {
        return "Animal: " + this.name;
    }
}

class Dog extends Animal {
    String breed;

    constructor(String name, String breed) {
        super(name);
        this.breed = breed;
    }

    // ERROR: Wrong parameter type - should be int, not String
    function makeSound(String loudness): void {
        print("Dog barks " + loudness);
    }

    // ERROR: Wrong return type - should return String, not void
    function getInfo(): void {
        print("Dog: " + this.name + ", Breed: " + this.breed);
    }
}

// This should never execute due to invalid override error
Dog myDog = new Dog("Max", "Labrador");
