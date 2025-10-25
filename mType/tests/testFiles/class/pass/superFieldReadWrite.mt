// Test super field read and write operations

class Animal {
    protected string name;
    protected int age;
    protected bool isAlive;

    constructor(string n, int a) {
        name = n;
        age = a;
        isAlive = true;
    }

    public function getInfo(): string {
        return name + " is " + age + " years old";
    }
}

class Dog extends Animal {
    private string breed;

    constructor(string n, int a, string b): super(n, a) {
        breed = b;
    }

    // Read super fields
    public function readSuperFields(): string {
        return "Name: " + super.name + ", Age: " + super.age + ", Alive: " + super.isAlive;
    }

    // Write to super fields
    public function celebrateBirthday(): void {
        super.age = super.age + 1;
        print(super.name + " is now " + super.age + " years old!");
    }

    // Mix of super field access and local fields
    public function getFullInfo(): string {
        return super.name + " (" + breed + "), age " + super.age;
    }

    // Modify multiple super fields
    public function rename(string newName): void {
        super.name = newName;
    }

    public function markDeceased(): void {
        super.isAlive = false;
    }
}

function main(): void {
    print("Testing super field read and write");

    Dog dog = new Dog("Buddy", 3, "Golden Retriever");

    // Test reading super fields
    print(dog.readSuperFields());

    // Test writing to super fields
    dog.celebrateBirthday();

    // Verify changes persisted
    print(dog.readSuperFields());

    // Test full info with mixed access
    print(dog.getFullInfo());

    // Test renaming
    dog.rename("Max");
    print(dog.getFullInfo());

    // Test boolean field modification
    dog.markDeceased();
    print(dog.readSuperFields());

    print("All super field operations completed successfully");
}

main();
