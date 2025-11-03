// Test casting to/from abstract class
abstract class Animal {
    public abstract function makeSound(): string;

    public function getType(): string {
        return "Animal";
    }
}

class Dog extends Animal {
    public function makeSound(): string {
        return "Woof";
    }
}

class Cat extends Animal {
    public function makeSound(): string {
        return "Meow";
    }
}

function main(): void {
    Dog dog = new Dog();
    Animal animal = dog;  // Upcast to abstract
    print(animal.makeSound());  // Should call Dog's implementation
    print(animal.getType());

    Cat cat = new Cat();
    Animal animal2 = cat;
    print(animal2.makeSound());  // Should call Cat's implementation
}

main();
