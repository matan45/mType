// Test: Downcast from parent to child class
class Animal {
    public string name;
    public constructor(string n) { this.name = n; }
}

class Dog extends Animal {
    public string breed;

    public constructor(string n, string b):super(n)  {
        this.breed = b;
    }

    public function bark(): void {
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
