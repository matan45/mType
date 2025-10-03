// Test: Method override - Child class overrides parent methods
// Expected: Pass - demonstrates method overriding and super.method() calls

class Animal {
    string name;

    constructor(string name) {
        this.name = name;
    }

    function sound(): void {
        print("Animal makes a sound");
    }

    function getInfo(): string {
        return "Animal: " + this.name;
    }
}

class Dog extends Animal {
    string breed;

    constructor(string name, string breed) {
        super(name);
        this.breed = breed;
    }

    function sound(): void {
        super.sound();  // Call parent method
        print("Dog barks: Woof Woof!");
    }

    function getInfo(): string {
        string parentInfo = super.getInfo();  // Call parent method for base info
        return parentInfo + ", Breed: " + this.breed;
    }
}

// Test method overriding
Dog dog = new Dog("Max", "Labrador");
dog.sound();  // Should print both parent and child messages
print(dog.getInfo());  // Should print combined info
