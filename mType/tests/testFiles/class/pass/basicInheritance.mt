// Test: Basic inheritance - Simple parent/child relationship
// Expected: Pass - demonstrates basic extends syntax and inherited fields

class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    public function speak(): void {
        print("Animal makes a sound");
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }

    public function getInfo(): string {
        return this.name + " is a " + this.breed;
    }
}

// Test basic inheritance
Dog dog = new Dog("Buddy", "Golden Retriever");
dog.speak();  // Should print "Animal makes a sound"
print(dog.getInfo());  // Should print "Buddy is a Golden Retriever"
