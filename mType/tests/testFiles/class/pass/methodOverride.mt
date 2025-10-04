// Test: Method override - Child class overrides parent methods
// Expected: Pass - demonstrates method overriding and super.method() calls

class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    public function sound(): void {
        print("Animal makes a sound");
    }

    public function getInfo(): string {
        return "Animal: " + this.name;
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, string breed): super(name) {
        this.breed = breed;
    }

    public function sound(): void {
        super.sound();  // Call parent method
        print("Dog barks: Woof Woof!");
    }

    public function getInfo(): string {
        string parentInfo = super.getInfo();  // Call parent method for base info
        return parentInfo + ", Breed: " + this.breed;
    }
}

// Test method overriding
Dog dog = new Dog("Max", "Labrador");
dog.sound();  // Should print both parent and child messages
print(dog.getInfo());  // Should print combined info
