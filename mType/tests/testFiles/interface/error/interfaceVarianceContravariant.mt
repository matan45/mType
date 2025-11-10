// Test invalid contravariance with interfaces
// @Throw

interface Consumer<T> {
    func consume(item: T): void;
}

class Animal {
    var name: String;

    func init(name: String) {
        this.name = name;
    }
}

class Dog extends Animal {
    func init(name: String) {
        super.init(name);
    }
}

class AnimalConsumer implements Consumer<Animal> {
    func consume(item: Animal): void {
        print(item.name);
    }
}

class DogShelter {
    var consumer: Consumer<Dog>;

    // Error: Cannot assign Consumer<Animal> to Consumer<Dog>
    // This would be unsafe as we could pass a Cat to consume(Dog)
    func init(consumer: Consumer<Animal>) {
        this.consumer = consumer;  // Type error
    }
}

var animalConsumer = new AnimalConsumer();
var shelter = new DogShelter(animalConsumer);
