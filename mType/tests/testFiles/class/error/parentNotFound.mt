// Test: Parent not found - Child extends undefined parent class
// Expected: Error - parent class does not exist

class Dog extends Animal {
    String breed;

    constructor(String name, String breed) {
        super(name);
        this.breed = breed;
    }

    function bark(): void {
        print("Woof!");
    }
}

// This should never execute due to undefined parent class error
Dog myDog = new Dog("Buddy", "Golden Retriever");
