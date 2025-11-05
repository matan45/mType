// Test two interfaces same method, different covariant returns
// @Script

class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }
}

class Dog extends Animal {
    public constructor(string name) : super(name) {
    }

    public function bark(): void {
        print("Woof!");
    }
}


interface DogFactory {
    function create(): Dog;
}

// Covariant return types - Dog is subtype of Animal
class PetFactory implements DogFactory {
    public function create(): Dog {
        // Returns Dog, which satisfies both Animal and Dog requirements
        return new Dog("Buddy");
    }
}

PetFactory factory = new PetFactory();
Animal animal = factory.create();
Dog dog = factory.create();

print(animal.name);  // Should print "Buddy"
print(dog.name);     // Should print "Buddy"
dog.bark();
