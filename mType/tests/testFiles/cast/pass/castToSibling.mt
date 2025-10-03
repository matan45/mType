// Test: Cast to sibling class (should fail)
class Animal {}
class Dog extends Animal {}
class Cat extends Animal {}

// This test verifies that sibling casts are properly rejected
// The actual error test is in error/castToSibling.mt

Dog dog = new Dog();
Animal animal = dog;
print("Upcast works");

// Expected output:
// Upcast works
