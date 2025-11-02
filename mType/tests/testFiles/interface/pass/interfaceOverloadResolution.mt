// Test overload resolution with interface types
// @Script

interface Animal {
    func speak(): String;
}

class Dog implements Animal {
    func speak(): String {
        return "Woof";
    }
}

class Cat implements Animal {
    func speak(): String {
        return "Meow";
    }
}

class AnimalHandler {
    // Overloaded methods with different interface types
    func handle(animal: Animal): void {
        print("Handling generic animal: " + animal.speak());
    }

    func handle(animal: Animal, times: Int): void {
        var i = 0;
        while (i < times) {
            print(animal.speak());
            i = i + 1;
        }
    }

    // Specific handling
    func handlePair(a1: Animal, a2: Animal): void {
        print(a1.speak() + " and " + a2.speak());
    }
}

var handler = new AnimalHandler();
var dog = new Dog();
var cat = new Cat();

handler.handle(dog);           // Should print "Handling generic animal: Woof"
handler.handle(cat, 2);        // Should print "Meow" twice
handler.handlePair(dog, cat);  // Should print "Woof and Meow"
