// Test: Cast in method call
class Animal {
    function makeSound(): void {
        print("Generic sound");
    }
}

class Dog extends Animal {
    function makeSound(): void {
        print("Woof");
    }
}

function processAnimal(Animal a): void {
    a.makeSound();
}

Dog dog = new Dog();
processAnimal((Animal)dog);  // Cast in method call
// Expected: Woof

// Expected output:
// Woof
