// Test parameter contravariance error - parameters cannot be contravariant in overriding
class Animal {
    string name;

    constructor(string n) {
        this.name = n;
    }
}

class Dog extends Animal {
    constructor(string n) {
        super(n);
    }
}

interface AnimalHandler {
    function handle(Dog dog): void;
}

// Error: Cannot use Animal (supertype) where Dog (subtype) is expected
// Parameters are contravariant, but mType should reject this
class BadHandler implements AnimalHandler {
    // Error: parameter type should be Dog, not Animal (supertype)
    public function handle(Animal animal): void {
        print("Handling: " + animal.name);
    }
}

BadHandler handler = new BadHandler();
Dog dog = new Dog("Buddy");
handler.handle(dog);
print("This should fail");
