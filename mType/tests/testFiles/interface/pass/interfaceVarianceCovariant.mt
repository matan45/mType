// Test covariant type usage with interfaces
// @Script

interface Producer<T> {
    function produce(): T;
}

class Animal {
    private string name;

    public constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }
}

class Dog extends Animal {
    public constructor(string name) : super(name) {
    }

    public function bark(): void {
        print("Woof!");
    }
}

class DogProducer implements Producer<Dog> {
    public constructor() {
    }

    public function produce(): Dog {
        return new Dog("Buddy");
    }
}

// Covariant usage - Producer<Dog> is used where Producer<Animal> would work
// in terms of reading (producing) values
class AnimalShelter {
    private Producer<Dog> producer;

    public constructor(Producer<Dog> producer) {
        this.producer = producer;
    }

    public function getAnimal(): Animal {
        // Covariant - Dog is a subtype of Animal
        return this.producer.produce();
    }
}

DogProducer dogProducer = new DogProducer();
AnimalShelter shelter = new AnimalShelter(dogProducer);
Animal animal = shelter.getAnimal();
print(animal.getName());  // Should print "Buddy"
