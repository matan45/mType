// Test: Invalid super() - super() call in wrong place or context
// Expected: Error - super() must be first statement in constructor

class Animal {
    String name;

    constructor(String name) {
        this.name = name;
    }
}

class Dog extends Animal {
    String breed;

    constructor(String name, String breed) {
        // ERROR: super() is not the first statement
        this.breed = breed;
        super(name);  // This should be first!
    }

    // ERROR: super() cannot be called in a regular method
    function resetName(String newName): void {
        super(newName);  // Invalid - super() only in constructors
    }
}

class Cat extends Animal {
    int age;

    constructor(String name, int age) {
        this.age = age;
        super(name);  // ERROR: super() must be first statement
    }
}

// This should never execute due to invalid super() placement errors
Dog myDog = new Dog("Max", "Beagle");
