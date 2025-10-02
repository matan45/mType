// Test: Basic inheritance - Simple parent/child relationship
// Expected: Pass - demonstrates basic extends syntax and inherited fields

class Animal {
    String name;

    constructor(String name) {
        this.name = name;
    }

    function speak(): void {
        print("Animal makes a sound");
    }
}

class Dog extends Animal {
    String breed;

    constructor(String name, String breed) {
        super(name);
        this.breed = breed;
    }

    function getInfo(): String {
        return this.name + " is a " + this.breed;
    }
}

// Test basic inheritance
Dog dog = new Dog("Buddy", "Golden Retriever");
dog.speak();  // Should print "Animal makes a sound"
print(dog.getInfo());  // Should print "Buddy is a Golden Retriever"
