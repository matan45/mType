import * from "../../lib/primitives/Int.mt";

// Test cast with upper bounded generics (<T extends Parent>)
class Animal {
    public function makeSound(): void {
        print("Animal sound");
    }
}

class Dog extends Animal {
    public function makeSound(): void {
        print("Bark");
    }

    public function wagTail(): void {
        print("Wagging tail");
    }
}

class Cat extends Animal {
    public function makeSound(): void {
        print("Meow");
    }
}

class Container<T extends Animal> {
    T item;

    public function setItem(T newItem): void {
        item = newItem;
    }

    public function getItem(): T {
        return item;
    }
}

function main(): void {
    // Create container with Dog (bounded by Animal)
    Container<Dog> dogContainer = new Container<Dog>();
    dogContainer.setItem(new Dog());

    // Cast to base bounded type
    Container<Animal> animalContainer = dogContainer as Container<Animal>;

    // Get and use the item
    Animal animal = animalContainer.getItem();
    animal.makeSound();

    // Test with Cat
    Container<Cat> catContainer = new Container<Cat>();
    catContainer.setItem(new Cat());
    Cat cat = catContainer.getItem();
    cat.makeSound();

    print("Bounded generics cast successful");
}

main();
