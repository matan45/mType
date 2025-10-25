// Test covariant return types in method overrides
// Child method can return a more specific type than parent

class Animal {
    string name;

    constructor(string n) {
        name = n;
    }

    public function create(): Animal {
        return new Animal("Generic Animal");
    }

    public function getName(): string {
        return name;
    }
}

class Dog extends Animal {
    string breed;

    constructor(string n, string b):super(n) {
        breed = b;
    }

    // Covariant return: Dog instead of Animal
    public function create(): Dog {
        return new Dog("New Dog", "Mixed");
    }

    public function getBreed(): string {
        return breed;
    }
}

class Cat extends Animal {
    bool indoor;

    constructor(string n, bool ind):super(n) {
        indoor = ind;
    }

    // Covariant return: Cat instead of Animal
    public function create(): Cat {
        return new Cat("New Cat", true);
    }

    public function isIndoor(): bool {
        return indoor;
    }
}

function main(): void {
    print("Testing covariant return types");

    // Create instances
    Animal animal = new Animal("Generic");
    Dog dog = new Dog("Buddy", "Labrador");
    Cat cat = new Cat("Whiskers", true);

    print("animal.create().getName(): " + animal.create().getName());
    print("dog.create().getName(): " + dog.create().getName());
    print("dog.create().getBreed(): " + dog.create().getBreed());
    print("cat.create().getName(): " + cat.create().getName());
    print("cat.create().isIndoor(): " + cat.create().isIndoor());

    // Polymorphic calls
    Animal animalRef1 = dog;
    Animal createdDog = animalRef1.create();
    print("Polymorphic dog creation: " + createdDog.getName());

    Animal animalRef2 = cat;
    Animal createdCat = animalRef2.create();
    print("Polymorphic cat creation: " + createdCat.getName());

    print("Covariant return types test completed");
}

main();
