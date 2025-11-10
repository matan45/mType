import * from "../../lib/primitives/Int.mt";

// Test covariant generic cast (Producer<Child> to Producer<Parent>)
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

class Cat extends Animal {
    public function getType(): void {
        print("Cat");
    }
}

// Producer - only returns T (covariant position)
class Producer<T> {
    T item;

    public function Producer(T initialItem) {
        item = initialItem;
    }

    public function produce(): T {
        return item;
    }
}

function processProducer(Producer<Animal> producer): void {
    Animal animal = producer.produce();
    animal.getType();
}

function main(): void {
    // Create producer of Dog
    Dog dog = new Dog();
    Producer<Dog> dogProducer = new Producer<Dog>(dog);

    // Covariant cast: Producer<Dog> can be cast to Producer<Animal>
    // This is safe because we can only read from it
    Producer<Animal> animalProducer = dogProducer as Producer<Animal>;
    Animal animal = animalProducer.produce();
    animal.getType();

    // Create producer of Cat
    Cat cat = new Cat();
    Producer<Cat> catProducer = new Producer<Cat>(cat);
    Producer<Animal> animalProducer2 = catProducer as Producer<Animal>;

    // Use polymorphic function
    processProducer(animalProducer);
    processProducer(animalProducer2);

    print("Covariant cast successful");
}

main();
