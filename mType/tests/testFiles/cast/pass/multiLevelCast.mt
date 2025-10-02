// Test: Multi-level inheritance casting
class Animal {
    void sound() { print("Some sound"); }
}

class Mammal extends Animal {
    void breathe() { print("Breathing"); }
}

class Dog extends Mammal {
    void bark() { print("Bark"); }
}

Dog dog = new Dog();

// Cast up two levels
Animal animal = (Animal)dog;
animal.sound();  // Expected: Some sound

// Cast to intermediate level
Mammal mammal = (Mammal)dog;
mammal.breathe();  // Expected: Breathing

// Cast back down
Dog castedDog = (Dog)mammal;
castedDog.bark();  // Expected: Bark

// Expected output:
// Some sound
// Breathing
// Bark
