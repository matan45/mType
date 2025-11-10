import * from "../../lib/primitives/Int.mt";

// Test multiple bounds casting
interface Flyable {
    public function fly(): void;
}

interface Swimmable {
    public function swim(): void;
}

class Animal {
    public function makeSound(): void {
        print("Animal sound");
    }
}

class Duck extends Animal implements Flyable, Swimmable {
    public function makeSound(): void {
        print("Quack");
    }

    public function fly(): void {
        print("Duck flying");
    }

    public function swim(): void {
        print("Duck swimming");
    }
}

class Container<T extends Animal & Flyable> {
    T item;

    public function setItem(T newItem): void {
        item = newItem;
    }

    public function getItem(): T {
        return item;
    }

    public function makeItemFly(): void {
        item.fly();
    }
}

function main(): void {
    // Create container with Duck (satisfies both Animal and Flyable)
    Container<Duck> duckContainer = new Container<Duck>();
    Duck duck = new Duck();
    duckContainer.setItem(duck);

    // Use the duck through container
    Duck retrievedDuck = duckContainer.getItem();
    retrievedDuck.makeSound();
    retrievedDuck.fly();
    retrievedDuck.swim();

    // Make item fly through container method
    duckContainer.makeItemFly();

    print("Multiple bounds cast successful");
}

main();
