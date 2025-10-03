// Test: Protected members accessible in subclasses
class Animal {
    protected string species;

    public Animal(string s) {
        species = s;
    }

    protected string getSpecies() {
        return species;
    }
}

class Dog extends Animal {
    public string name;

    public Dog(string n) {
        super("Canine");
        name = n;
    }

    public string getFullInfo() {
        // Accessing protected field and method from parent class
        return name + " is a " + getSpecies();
    }
}

Dog dog = new Dog("Buddy");
print(dog.getFullInfo());  // Expected: Buddy is a Canine
