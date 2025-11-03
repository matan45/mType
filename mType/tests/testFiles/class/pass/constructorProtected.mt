// Test: Protected constructor - accessible in subclasses
// Expected: Pass - demonstrates protected constructor usage

class Animal {
    protected string species;
    protected int age;

    // Protected constructor - only accessible in this class and subclasses
    protected constructor(string species, int age) {
        this.species = species;
        this.age = age;
        print("Animal constructor: " + species + ", age " + age);
    }

    public function getInfo():string {
        return this.species + " (age: " + this.age + ")";
    }
}

class Dog extends Animal {
    private string name;

    // Public constructor that calls protected parent constructor
    public constructor(string name, int age) : super("Dog", age) {
        this.name = name;
        print("Dog constructor: " + name);
    }

    public function getFullInfo(): string {
        return this.name + " the " + this.getInfo();
    }
}

class Cat extends Animal {
    private string name;
    private bool indoor;

    public constructor(string name, int age, bool indoor) : super("Cat", age) {
        this.name = name;
        this.indoor = indoor;
        print("Cat constructor: " + name + ", indoor: " + indoor);
    }

    public function getFullInfo(): string {
        string location = this.indoor ? "indoor" : "outdoor";
        return this.name + " the " + location + " " + this.getInfo();
    }
}

// Test protected constructor through subclasses
print("Creating a dog:");
Dog dog = new Dog("Buddy", 5);
print(dog.getFullInfo());

print("\nCreating a cat:");
Cat cat = new Cat("Whiskers", 3, true);
print(cat.getFullInfo());
