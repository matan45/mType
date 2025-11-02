import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test wildcard with upper bounds (? extends Type)
interface Animal {
    function makeSound(): string;
}

class Dog implements Animal {
    string name;

    constructor(string n) {
        name = n;
    }

    public function makeSound(): string {
        return "Woof";
    }

    public function getName(): string {
        return name;
    }
}

class Cat implements Animal {
    string name;

    constructor(string n) {
        name = n;
    }

    public function makeSound(): string {
        return "Meow";
    }

    public function getName(): string {
        return name;
    }
}

class Container<T extends Animal> {
    T item;

    constructor(T i) {
        item = i;
    }

    public function getItem(): T {
        return item;
    }

    public function sound(): string {
        return item.makeSound();
    }
}

// Function that accepts any Container with Animal upper bound
function <T extends Animal> printAnimalSound(Container<T> container): void {
    print("Animal says: " + container.sound());
}

function main(): void {
    print("Testing wildcard with upper bounds");

    Dog dog = new Dog("Buddy");
    Container<Dog> dogContainer = new Container<Dog>(dog);
    print("Dog name: " + dogContainer.getItem().getName());
    printAnimalSound<Dog>(dogContainer);

    Cat cat = new Cat("Whiskers");
    Container<Cat> catContainer = new Container<Cat>(cat);
    print("Cat name: " + catContainer.getItem().getName());
    printAnimalSound<Cat>(catContainer);

    print("Upper bound wildcard test completed");
}

main();
