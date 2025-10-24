// Test: Abstract class inheritance chain
// Abstract classes can extend other abstract classes

abstract class Animal {
    abstract function makeSound(): string;

    public function breathe(): void {
        print("Breathing...");
    }
}

abstract class Mammal extends Animal {
    abstract function getFurColor(): string;

    public function warmBlooded(): bool {
        return true;
    }
}

class Dog extends Mammal {
    public function makeSound(): string {
        return "Woof!";
    }

    public function getFurColor(): string {
        return "Brown";
    }
}

class Cat extends Mammal {
    public function makeSound(): string {
        return "Meow!";
    }

    public function getFurColor(): string {
        return "Orange";
    }
}

// Test the implementation
Dog dog = new Dog();
print("Dog says: " + dog.makeSound());
print("Dog fur: " + dog.getFurColor());
print("Warm blooded: " + dog.warmBlooded());
dog.breathe();

Cat cat = new Cat();
print("Cat says: " + cat.makeSound());
print("Cat fur: " + cat.getFurColor());
