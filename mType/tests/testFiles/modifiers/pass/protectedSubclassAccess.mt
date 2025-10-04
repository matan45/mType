// Test: Protected members accessible in subclasses
class Animal {
    protected string species;

    public constructor(string s) {
        species = s;
    }

    protected function getSpecies(): string {
        return species;
    }
}

class Dog extends Animal {
    public string name;

    public constructor(string n):super("Canine") {
        name = n;
    }

    public function getFullInfo(): string {
        // Accessing protected field and method from parent class
        return name + " is a " + super.getSpecies();
    }
}

Dog dog = new Dog("Buddy");
print(dog.getFullInfo());  // Expected: Buddy is a Canine
