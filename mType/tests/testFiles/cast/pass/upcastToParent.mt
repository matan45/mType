// Test: Upcast from child to parent class
class Animal {
    public string name;

    public constructor(string n) {
        this.name = n;
    }

    public function speak(): void {
        print("Animal speaks");
    }
}

class Dog extends Animal {
    public constructor(string n):super(n) {
	}

    public function speak():void {
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
