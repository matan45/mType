@Script
// Test casting to/from abstract class
abstract class Animal {
    abstract fn makeSound(): String;

    fn getType(): String {
        return "Animal";
    }
}

class Dog extends Animal {
    fn makeSound(): String {
        return "Woof";
    }
}

class Cat extends Animal {
    fn makeSound(): String {
        return "Meow";
    }
}

fn main() {
    let dog: Dog = new Dog();
    let animal: Animal = dog as Animal;  // Upcast to abstract
    print(animal.makeSound());  // Should call Dog's implementation
    print(animal.getType());

    let cat: Cat = new Cat();
    let animal2: Animal = cat as Animal;
    print(animal2.makeSound());  // Should call Cat's implementation
}
