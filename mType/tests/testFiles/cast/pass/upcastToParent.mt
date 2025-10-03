// Test: Upcast from child to parent class
class Animal {
    string name;

    constructor(string n) {
        this.name = n;
    }

    function speak(): void {
        print("Animal speaks");
    }
}

class Dog extends Animal {
    constructor(string n) {
	 super(n);
	}

    function speak():void {
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
