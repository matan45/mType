// Test: isClassOf with parent class
class Animal {}
class Dog extends Animal {}

Dog dog = new Dog();
print(dog isClassOf Dog);    // true
print(dog isClassOf Animal); // true

Animal animal = new Animal();
print(animal isClassOf Animal); // true
print(animal isClassOf Dog);    // false

// Expected output:
// true
// true
// true
// false
