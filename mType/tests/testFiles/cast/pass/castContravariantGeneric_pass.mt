import * from "../../lib/primitives/Int.mt";

// Test contravariant generic cast (Consumer<Parent> to Consumer<Child>)
class Animal {
    public function getType(): void {
        print("Processing Animal");
    }
}

class Dog extends Animal {
    public function getType(): void {
        print("Processing Dog");
    }
}

class Cat extends Animal {
    public function getType(): void {
        print("Processing Cat");
    }
}

// Consumer - only accepts T (contravariant position)
class Consumer<T> {
    public function consume(T item): void {
        item.getType();
        print("Item consumed");
    }
}

function main(): void {
    // Create consumer that accepts Animal (base type)
    Consumer<Animal> animalConsumer = new Consumer<Animal>();

    // Contravariant cast: Consumer<Animal> can be cast to Consumer<Dog>
    // This is safe because if it can consume Animal, it can consume Dog
    Consumer<Dog> dogConsumer = animalConsumer as Consumer<Dog>;

    // Use the dog consumer
    Dog dog = new Dog();
    dogConsumer.consume(dog);

    // Same for Cat
    Consumer<Cat> catConsumer = animalConsumer as Consumer<Cat>;
    Cat cat = new Cat();
    catConsumer.consume(cat);

    print("Contravariant cast successful");
}

main();
