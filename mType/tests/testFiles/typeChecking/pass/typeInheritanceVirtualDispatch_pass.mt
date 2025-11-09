// Test: Virtual method dispatch (polymorphism)
// Expected: Pass - demonstrates dynamic dispatch of overridden methods

class Animal {
    protected string name;
    protected int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function speak(): void {
        print(this.name + " makes a sound");
    }

    public function move(): void {
        print(this.name + " moves");
    }

    public function getName(): string {
        return this.name;
    }

    public function getAge(): int {
        return this.age;
    }
}

class Dog extends Animal {
    private string breed;

    public constructor(string name, int age, string breed) : super(name, age) {
        this.breed = breed;
    }

    public function speak(): void {
        print(this.name + " barks: Woof! Woof!");
    }

    public function move(): void {
        print(this.name + " runs on four legs");
    }

    public function getBreed(): string {
        return this.breed;
    }
}

class Cat extends Animal {
    private bool isIndoor;

    public constructor(string name, int age, bool isIndoor) : super(name, age) {
        this.isIndoor = isIndoor;
    }

    public function speak(): void {
        print(this.name + " meows: Meow!");
    }

    public function move(): void {
        print(this.name + " walks gracefully");
    }

    public function getIsIndoor(): bool {
        return this.isIndoor;
    }
}

class Bird extends Animal {
    private int wingspan;

    public constructor(string name, int age, int wingspan) : super(name, age) {
        this.wingspan = wingspan;
    }

    public function speak(): void {
        print(this.name + " chirps: Tweet! Tweet!");
    }

    public function move(): void {
        print(this.name + " flies with wingspan " + this.wingspan);
    }

    public function getWingspan(): int {
        return this.wingspan;
    }
}

// Test 1: Direct polymorphic calls
print("Test 1: Direct polymorphic dispatch");
Animal a1 = new Dog("Buddy", 3, "Golden Retriever");
Animal a2 = new Cat("Whiskers", 5, true);
Animal a3 = new Bird("Tweety", 1, 25);

a1.speak();  // Should call Dog's speak()
a1.move();   // Should call Dog's move()

a2.speak();  // Should call Cat's speak()
a2.move();   // Should call Cat's move()

a3.speak();  // Should call Bird's speak()
a3.move();   // Should call Bird's move()

// Test 2: Polymorphic dispatch in array
print("\nTest 2: Array polymorphic dispatch");
Animal[] animals = new Animal[4];
animals[0] = new Dog("Max", 4, "Labrador");
animals[1] = new Cat("Luna", 2, false);
animals[2] = new Bird("Robin", 1, 30);
animals[3] = new Dog("Charlie", 5, "Beagle");

int i = 0;
while (i < 4) {
    print("\nAnimal " + i + ": " + animals[i].getName() + ", Age: " + animals[i].getAge());
    animals[i].speak();
    animals[i].move();
    i = i + 1;
}

// Test 3: Polymorphic dispatch through function parameter
function performActions(Animal animal): void {
    print("\nPerforming actions for: " + animal.getName());
    animal.speak();
    animal.move();
}

print("\nTest 3: Function parameter polymorphic dispatch");
performActions(new Dog("Daisy", 2, "Poodle"));
performActions(new Cat("Shadow", 4, true));
performActions(new Bird("Eagle", 3, 180));

// Test 4: Multiple levels of virtual dispatch
print("\nTest 4: Mixed type handling");
Animal[] mixedAnimals = new Animal[3];
mixedAnimals[0] = new Animal("Generic", 1);
mixedAnimals[1] = new Dog("Rex", 6, "German Shepherd");
mixedAnimals[2] = new Cat("Mittens", 3, true);

int j = 0;
while (j < 3) {
    print("\nProcessing: " + mixedAnimals[j].getName());
    mixedAnimals[j].speak();
    mixedAnimals[j].move();
    j = j + 1;
}

print("\nVirtual dispatch tests completed");
