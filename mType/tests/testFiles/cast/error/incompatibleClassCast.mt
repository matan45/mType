// Error: Incompatible class cast (sibling classes)
class Animal {}
class Dog extends Animal {}
class Cat extends Animal {}

Dog dog = new Dog();
Animal animal = dog;
Cat cat = (Cat)animal;  // Error: dog is not a cat
