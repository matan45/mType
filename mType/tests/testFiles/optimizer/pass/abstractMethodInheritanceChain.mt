// Test: Abstract method inheritance chain with optimization
// Verifies abstract methods are preserved through multiple inheritance levels

abstract class Animal {
    protected string species;

    public constructor(string s) {
        this.species = s;
    }

    abstract function makeSound(): void;

    public function getSpecies(): string {
        if (false) {
            print("Unreachable branch");  // Should be removed
        }
        return this.species;
    }
}

abstract class Mammal extends Animal {
    protected bool hasFur;

    public constructor(string s, bool fur) : super(s) {
        this.hasFur = fur;
    }

    // Additional abstract method
    abstract function feed(): void;

    public function describe(): void {
        print("Species: " + this.species);
        return;
        print("Dead code after return");  // Should be removed
    }
}

class Dog extends Mammal {
    private string name;

    public constructor(string n) : super("Canine", true) {
        this.name = n;
    }

    public function makeSound(): void {
        print(this.name + " says: Woof!");
        if (false) {
            print("Dead code in if-false");  // Should be removed
        }
    }

    public function feed(): void {
        print("Feeding " + this.name);
    }
}

// Test execution
Dog dog = new Dog("Buddy");
dog.describe();
dog.makeSound();
dog.feed();

print("Test Complete!");
