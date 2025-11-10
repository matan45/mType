import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Covariant type usage - reading from generic types
class Animal {
    String name;

    public constructor(String n) {
        name = n;
    }

    public function getName(): String {
        return name;
    }
}

class Dog extends Animal {
    public constructor(String n) : super(n) {
    }
}

class Producer<T> {
    T value;

    public function setValue(T v): void {
        value = v;
    }

    public function getValue(): T {
        return value;
    }
}

function printAnimal(Producer<Animal> producer): void {
    Animal animal = producer.getValue();
    print(animal.getName().toString());
}

function main(): void {
    Producer<Dog> dogProducer = new Producer<Dog>();
    dogProducer.setValue(new Dog(new String("Rex")));

    // Covariant use - can read Dog as Animal
    printAnimal(dogProducer);
}

main();
