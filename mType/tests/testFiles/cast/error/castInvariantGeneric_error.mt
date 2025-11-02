import * from "../../lib/primitives/Int.mt";

// Test invariant generic cast error case
class Animal {
    public function getType(): void {
        print("Animal");
    }
}

class Dog extends Animal {
    public function getType(): void {
        print("Dog");
    }
}

// Container - both reads and writes T (invariant)
class Container<T> {
    T item;

    public function setItem(T newItem): void {
        item = newItem;
    }

    public function getItem(): T {
        return item;
    }
}

function main(): void {
    // Create container with Dog
    Container<Dog> dogContainer = new Container<Dog>();
    dogContainer.setItem(new Dog());

    // ERROR: Cannot cast Container<Dog> to Container<Animal>
    // This is unsafe because Container allows both reading and writing
    // If we allowed this, we could put a Cat into a Container<Dog>
    Container<Animal> animalContainer = dogContainer as Container<Animal>;

    // This would be type-unsafe
    animalContainer.setItem(new Animal());

    print("This should not execute");
}

main();
