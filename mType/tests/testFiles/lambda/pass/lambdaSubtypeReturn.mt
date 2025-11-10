// Returning subclass when parent expected test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class Animal {
    public function makeSound() : String {
        return "Some sound";
    }

    public function getType() : String {
        return "Animal";
    }
}

class Dog extends Animal {
    public function makeSound() : String {
        return "Woof";
    }

    public function getType() : String {
        return "Dog";
    }
}

class Cat extends Animal {
    public function makeSound() : String {
        return "Meow";
    }

    public function getType() : String {
        return "Cat";
    }
}

print("=== Subtype Return Test ===");

// Lambda declared to return Animal, but returns Dog (subtype)
Function<Int, Animal> animalFactory = x -> {
    if (x == 1) {
        return new Dog();
    } else if (x == 2) {
        return new Cat();
    } else {
        return new Animal();
    }
};

Animal a1 = animalFactory.apply(1);
Animal a2 = animalFactory.apply(2);
Animal a3 = animalFactory.apply(3);

print("Animal 1: " + a1.getType() + " says " + a1.makeSound());
print("Animal 2: " + a2.getType() + " says " + a2.makeSound());
print("Animal 3: " + a3.getType() + " says " + a3.makeSound());

// Lambda always returning subtype
Function<String, Animal> dogFactory = name -> {
    return new Dog();
};

Animal dog = dogFactory.apply("Rex");
print("Dog factory: " + dog.getType() + " says " + dog.makeSound());

print("Subtype return complete");
