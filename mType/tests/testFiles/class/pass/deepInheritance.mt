// Test: Deep inheritance - Grandparent → Parent → Child hierarchy
// Expected: Pass - demonstrates multi-level inheritance

class Being {
    string kingdom;

    constructor(string kingdom) {
        this.kingdom = kingdom;
        print("Being constructor: Kingdom " + kingdom);
    }

    function exist(): void {
        print("Being exists in kingdom: " + this.kingdom);
    }
}

class Animal extends Being {
    string species;

    constructor(string species): super("Animalia") {
        this.species = species;
        print("Animal constructor: Species " + species);
    }

    function move(): void {
        print("Animal moves");
    }
}

class Mammal extends Animal {
    bool hasFur;

    constructor(string species, bool hasFur): super(species) {
        this.hasFur = hasFur;
        print("Mammal constructor: Has fur? " + hasFur);
    }

    function nurse(): void {
        print("Mammal nurses young");
    }
}

class Dog extends Mammal {
    string breed;

    constructor(string breed): super("Canis familiaris", true) {
        this.breed = breed;
        print("Dog constructor: Breed " + breed);
    }

    function bark(): void {
        print("Dog barks!");
    }
}

// Test deep inheritance
print("=== Creating a Dog ===");
Dog myDog = new Dog("Beagle");

print("\n=== Testing inherited methods ===");
myDog.exist();   // From Being (grandparent's grandparent)
myDog.move();    // From Animal (grandparent)
myDog.nurse();   // From Mammal (parent)
myDog.bark();    // From Dog (self)

print("\n=== Testing inherited fields ===");
print("Kingdom: " + myDog.kingdom);
print("Species: " + myDog.species);
print("Has fur: " + myDog.hasFur);
print("Breed: " + myDog.breed);
