// Test: Multi-level inheritance casting
class Animal {
    public function sound(): void { print("Some sound"); }
}

class Mammal extends Animal {
    public function breathe(): void { print("Breathing"); }
}

class Dog extends Mammal {
    public function bark(): void { print("Bark"); }
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
