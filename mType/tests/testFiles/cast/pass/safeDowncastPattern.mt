// Test: Safe downcast pattern (check before cast)
class Animal {}
class Dog extends Animal {
    public function bark(): void { print("Woof"); }
}

Animal animal = new Dog();

if (animal isClassOf Dog) {
    Dog dog = (Dog)animal;
    dog.bark();
}

// Expected output:
// Woof
