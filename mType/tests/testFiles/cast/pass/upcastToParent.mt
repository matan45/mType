// Test: Upcast from child to parent class
class Animal {
    string name;

    Animal(string n) {
        this.name = n;
    }

    void speak() {
        print("Animal speaks");
    }
}

class Dog extends Animal {
    Dog(string n) : super(n) {}

    void speak() {
        print("Dog barks");
    }
}

Dog dog = new Dog("Buddy");
Animal animal = (Animal)dog;  // Upcast
animal.speak();  // Expected: Dog barks (polymorphism)
print(animal.name);  // Expected: Buddy

// Expected output:
// Dog barks
// Buddy
