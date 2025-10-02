// Test: Downcast from parent to child class
class Animal {
    string name;
    Animal(string n) { this.name = n; }
}

class Dog extends Animal {
    string breed;

    Dog(string n, string b) : super(n) {
        this.breed = b;
    }

    void bark() {
        print("Woof!");
    }
}

Dog originalDog = new Dog("Buddy", "Golden");
Animal animal = originalDog;  // Implicit upcast
Dog castedDog = (Dog)animal;   // Explicit downcast
castedDog.bark();  // Expected: Woof!
print(castedDog.breed);  // Expected: Golden

// Expected output:
// Woof!
// Golden
