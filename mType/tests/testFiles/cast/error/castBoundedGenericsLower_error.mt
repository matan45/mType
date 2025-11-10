import * from "../../lib/primitives/Int.mt";

// Test cast with incompatible lower bound - should fail
class Animal {
    public function makeSound(): void {
        print("Animal sound");
    }
}

class Dog extends Animal {
    public function makeSound(): void {
        print("Bark");
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
    // Create container with Animal
    Container<Animal> animalContainer = new Container<Animal>();
    animalContainer.setItem(new Animal());

    // ERROR: Cannot cast from Container<Animal> to Container<Dog>
    // This violates type safety - we cannot guarantee the container holds a Dog
    Container<Dog> dogContainer = animalContainer as Container<Dog>;

    Dog dog = dogContainer.getItem();
    dog.makeSound();

    print("This should not execute");
}

main();
