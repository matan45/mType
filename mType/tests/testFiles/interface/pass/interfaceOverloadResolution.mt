// Test overload resolution with interface types
// @Script

interface Animal {
    function speak(): string;
}

class Dog implements Animal {
    public function speak(): string {
        return "Woof";
    }
}

class Cat implements Animal {
    public function speak(): string {
        return "Meow";
    }
}

class AnimalHandler {
    // Handle a single animal
    public function handle(Animal animal): void {
        print("Handling generic animal: " + animal.speak());
    }

    // Handle animal multiple times
    public function handleMultiple(Animal animal, int times): void {
        int i = 0;
        while (i < times) {
            print(animal.speak());
            i = i + 1;
        }
    }

    // Specific handling
    public function handlePair(Animal a1, Animal a2): void {
        print(a1.speak() + " and " + a2.speak());
    }
}

AnimalHandler handler = new AnimalHandler();
Dog dog = new Dog();
Cat cat = new Cat();

handler.handle(dog);               // Should print "Handling generic animal: Woof"
handler.handleMultiple(cat, 2);  // Should print "Meow" twice
handler.handlePair(dog, cat);    // Should print "Woof and Meow"
