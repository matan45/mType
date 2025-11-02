// Test two interfaces same method, different covariant returns
// @Script

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

interface AnimalFactory {
    func create(): Animal;
}

interface DogFactory {
    func create(): Dog;
}

// Covariant return types - Dog is subtype of Animal
class PetFactory implements AnimalFactory, DogFactory {
    func create(): Dog {
        // Returns Dog, which satisfies both Animal and Dog requirements
        return new Dog("Buddy");
    }
}

var factory = new PetFactory();
var animal: Animal = factory.create();
var dog: Dog = factory.create();

print(animal.name);  // Should print "Buddy"
print(dog.name);     // Should print "Buddy"
dog.bark();
