// Test instanceof check with interfaces
// @Script

interface Animal {
    func speak(): String;
}

interface Flyable {
    func fly(): void;
}

class Dog implements Animal {
    func speak(): String {
        return "Woof";
    }
}

class Bird implements Animal, Flyable {
    func speak(): String {
        return "Tweet";
    }

    func fly(): void {
        print("Flying!");
    }
}

func processAnimal(animal: Animal): void {
    print(animal.speak());

    // Check if animal is also flyable
    if (animal instanceof Flyable) {
        var flyable: Flyable = animal as Flyable;
        flyable.fly();
    } else {
        print("This animal cannot fly");
    }
}

var dog = new Dog();
var bird = new Bird();

processAnimal(dog);   // Should print "Woof" and "This animal cannot fly"
processAnimal(bird);  // Should print "Tweet" and "Flying!"
