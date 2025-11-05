// Lambda generic variance test (covariance/contravariance patterns)
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class Animal {
    public function makeSound() : string {
        return "Some sound";
    }
}

class Dog extends Animal {
    public function makeSound() : string {
        return "Woof";
    }
}

class Cat extends Animal {
    public function makeSound() : string {
        return "Meow";
    }
}

print("=== Generic Variance Test ===");

// Contravariance - accepting more general input
Function<Animal, String> soundMaker = a -> new String(a.makeSound());

Dog dog = new Dog();
Cat cat = new Cat();

print("Dog says: " + soundMaker.apply(dog));
print("Cat says: " + soundMaker.apply(cat));

// Covariance pattern - returning more specific type
Function<String, Animal> animalFactory = s -> {
    Animal result;
    if (s == "dog") {
        result = new Dog();
    } else {
        result = new Cat();
    }
    return result;
};

Animal a1 = animalFactory.apply("dog");
Animal a2 = animalFactory.apply("cat");
print("Factory dog: " + a1.makeSound());
print("Factory cat: " + a2.makeSound());

print("Variance complete");
