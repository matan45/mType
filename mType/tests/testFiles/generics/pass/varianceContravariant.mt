import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Contravariant type usage - writing to generic types
class Animal {
    String name;

    public function Animal(String n) {
        name = n;
    }
}

class Dog extends Animal {
    public function Dog(String n) {
        super(n);
    }
}

class Consumer<T> {
    T[] items;
    Int count;

    public function Consumer() {
        items = new T[10];
        count = new Int(0);
    }

    public function accept(T item): void {
        items[count.value] = item;
        count = new Int(count.value + 1);
    }

    public function getCount(): Int {
        return count;
    }
}

function main(): void {
    Consumer<Animal> animalConsumer = new Consumer<Animal>();

    // Contravariant use - can write Dog to Animal consumer
    animalConsumer.accept(new Dog(new String("Buddy")));
    animalConsumer.accept(new Animal(new String("Generic")));

    print("Accepted: " + animalConsumer.getCount());
}

main();
