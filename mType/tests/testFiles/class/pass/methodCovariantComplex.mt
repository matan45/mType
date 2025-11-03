// Test: Advanced covariant return types in inheritance hierarchy
// Expected: Pass - demonstrates covariant return types

class Animal {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function reproduce(): Animal {
        print("Animal.reproduce() -> Animal");
        return new Animal(this.name + "_offspring");
    }

    public function getName(): string {
        return this.name;
    }
}

class Mammal extends Animal {
    protected int gestationDays;

    public constructor(string name, int gestationDays) : super(name) {
        this.gestationDays = gestationDays;
    }

    // Covariant return type: overrides Animal.reproduce() but returns Mammal
    public function reproduce(): Mammal {
        print("Mammal.reproduce() -> Mammal");
        return new Mammal(this.name + "_baby", this.gestationDays);
    }

    public function getGestationDays(): int {
        return this.gestationDays;
    }
}

class Dog extends Mammal {
    protected string breed;

    public constructor(string name, string breed) : super(name, 63) {
        this.breed = breed;
    }

    // Covariant return type: overrides Mammal.reproduce() but returns Dog
    public function reproduce(): Dog {
        print("Dog.reproduce() -> Dog");
        return new Dog(this.name + "_puppy", this.breed);
    }

    public function getBreed(): string {
        return this.breed;
    }
}

// Test covariant return types
print("Test 1: Animal reproduction");
Animal a = new Animal("Generic");
Animal a2 = a.reproduce();
print("Offspring: " + a2.getName());

print("\nTest 2: Mammal reproduction");
Mammal m = new Mammal("Whale", 365);
Mammal m2 = m.reproduce();
print("Offspring: " + m2.getName() + ", gestation: " + m2.getGestationDays());

print("\nTest 3: Dog reproduction");
Dog d = new Dog("Buddy", "Labrador");
Dog d2 = d.reproduce();
print("Offspring: " + d2.getName() + ", breed: " + d2.getBreed() + ", gestation: " + d2.getGestationDays());

print("\nTest 4: Polymorphic call");
Animal polyAnimal = new Dog("Max", "Beagle");
Animal polyOffspring = polyAnimal.reproduce();
print("Polymorphic offspring: " + polyOffspring.getName());
