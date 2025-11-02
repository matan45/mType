// Test covariant type usage with interfaces
// @Script

interface Producer<T> {
    func produce(): T;
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

    func bark(): void {
        print("Woof!");
    }
}

class DogProducer implements Producer<Dog> {
    func produce(): Dog {
        return new Dog("Buddy");
    }
}

// Covariant usage - Producer<Dog> is used where Producer<Animal> would work
// in terms of reading (producing) values
class AnimalShelter {
    var producer: Producer<Dog>;

    func init(producer: Producer<Dog>) {
        this.producer = producer;
    }

    func getAnimal(): Animal {
        // Covariant - Dog is a subtype of Animal
        return this.producer.produce();
    }
}

var dogProducer = new DogProducer();
var shelter = new AnimalShelter(dogProducer);
var animal = shelter.getAnimal();
print(animal.name);  // Should print "Buddy"
