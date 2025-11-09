// Test: Type guards and type narrowing
// Expected: Pass - type narrowing through conditional checks

class Animal {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }

    public function makeSound(): void {
        print("Animal sound");
    }
}

class Dog extends Animal {
    private string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }

    public function makeSound(): void {
        print("Woof!");
    }

    public function getBreed(): string {
        return this.breed;
    }

    public function fetch(): void {
        print(this.name + " is fetching");
    }
}

class Cat extends Animal {
    private bool indoor;

    public constructor(string name, bool indoor) : super(name) {
        this.indoor = indoor;
    }

    public function makeSound(): void {
        print("Meow!");
    }

    public function isIndoor(): bool {
        return this.indoor;
    }

    public function scratch(): void {
        print(this.name + " is scratching");
    }
}

// Test 1: Type narrowing through polymorphic method calls
print("Test 1: Type narrowing with polymorphism");
Animal animal1 = new Dog("Buddy", "Golden Retriever");
animal1.makeSound();  // Should call Dog's makeSound

Animal animal2 = new Cat("Whiskers", true);
animal2.makeSound();  // Should call Cat's makeSound

// Test 2: Array with mixed types requiring type guards
print("\nTest 2: Mixed type array");
Animal[] animals = new Animal[3];
animals[0] = new Dog("Max", "Labrador");
animals[1] = new Cat("Luna", false);
animals[2] = new Animal("Unknown");

int i = 0;
while (i < 3) {
    print("Animal " + i + ": " + animals[i].getName());
    animals[i].makeSound();
    i = i + 1;
}

// Test 3: Type narrowing in function parameters
function processAnimal(Animal a): void {
    print("Processing: " + a.getName());
    a.makeSound();
}

print("\nTest 3: Function with type guard");
Dog myDog = new Dog("Rex", "German Shepherd");
processAnimal(myDog);

Cat myCat = new Cat("Shadow", true);
processAnimal(myCat);

// Test 4: Conditional type narrowing pattern
print("\nTest 4: Conditional type handling");
Animal testAnimal = new Dog("Charlie", "Beagle");
print("Animal name: " + testAnimal.getName());
testAnimal.makeSound();

// Reassignment with different type
testAnimal = new Cat("Mittens", true);
print("Animal name: " + testAnimal.getName());
testAnimal.makeSound();

print("\nType guard tests completed");
