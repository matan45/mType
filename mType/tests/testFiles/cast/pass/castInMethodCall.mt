// Test: Cast in method call
class Animal {
    void makeSound() {
        print("Generic sound");
    }
}

class Dog extends Animal {
    void makeSound() {
        print("Woof");
    }
}

void processAnimal(Animal a) {
    a.makeSound();
}

Dog dog = new Dog();
processAnimal((Animal)dog);  // Cast in method call
// Expected: Woof

// Expected output:
// Woof
