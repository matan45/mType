// Test: isClassOf in conditional
class Animal {}
class Dog extends Animal {}

Animal animal = new Dog();

if (animal isClassOf Dog) {
    print("It's a dog");
} else {
    print("Not a dog");
}

// Expected output:
// It's a dog
