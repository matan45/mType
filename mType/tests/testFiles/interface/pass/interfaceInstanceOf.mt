// Test instanceof check with interfaces
// @Script

interface Animal {
    function speak(): string;
}

interface Flyable {
    function fly(): void;
}

class Dog implements Animal {
    public function speak(): string {
        return "Woof";
    }
}

class Bird implements Animal, Flyable {
    public function speak(): string {
        return "Tweet";
    }

    public function fly(): void {
        print("Flying!");
    }
}

function processAnimal(Animal animal): void {
    print(animal.speak());

    // Check if animal is also flyable
    if (animal isClassOf Flyable) {
        Flyable flyable = (Flyable)animal;
        flyable.fly();
    } else {
        print("This animal cannot fly");
    }
}

Dog dog = new Dog();
Bird bird = new Bird();

processAnimal(dog);   // Should print "Woof" and "This animal cannot fly"
processAnimal(bird);  // Should print "Tweet" and "Flying!"
