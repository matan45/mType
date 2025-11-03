// Test: Explicit upcast from child to parent type
// Expected: Pass - demonstrates explicit upcasting

class Animal {
    protected string species;

    public constructor(string species) {
        this.species = species;
    }

    public function makeSound(): void {
        print("Animal sound");
    }

    public function getSpecies(): string {
        return this.species;
    }
}

class Dog extends Animal {
    private string breed;

    public constructor(string breed) : super("Canine") {
        this.breed = breed;
    }

    public function makeSound(): void {
        print("Woof!");
    }

    public function getBreed(): string {
        return this.breed;
    }
}

class Cat extends Animal {
    private int lives;

    public constructor() : super("Feline") {
        this.lives = 9;
    }

    public function makeSound(): void {
        print("Meow!");
    }

    public function getLives(): int {
        return this.lives;
    }
}

// Test explicit upcasting
print("Test 1: Dog to Animal upcast");
Dog dog = new Dog("Labrador");
Animal animal1 = dog;  // Implicit upcast
animal1.makeSound();   // Polymorphic call
print("Species: " + animal1.getSpecies());

print("\nTest 2: Cat to Animal upcast");
Cat cat = new Cat();
Animal animal2 = cat;  // Implicit upcast
animal2.makeSound();   // Polymorphic call
print("Species: " + animal2.getSpecies());

print("\nTest 3: Upcast in array");
Animal[] animals = new Animal[3];
animals[0] = new Dog("Beagle");
animals[1] = new Cat();
animals[2] = new Animal("Generic");

int i = 0;
while (i < 3) {
    print("Animal " + i + ":");
    animals[i].makeSound();
    i = i + 1;
}
