// Test class metadata introspection

import * from "../../lib/reflect/Class.mt";

abstract class Animal {
    public string name;
}

final class Dog extends Animal {
    public int age;

    public constructor(string name, int age) : super() {
        this.name = name;
        this.age = age;
    }
}

class Cat extends Animal {
    public constructor(string name) : super() {
        this.name = name;
    }
}

// Test abstract class
Class animalClass = Class::forName("Animal");
print("Animal is abstract: " + animalClass.isAbstract());
print("Animal is final: " + animalClass.isFinal());

// Test final class
Class dogClass = Class::forName("Dog");
print("Dog is abstract: " + dogClass.isAbstract());
print("Dog is final: " + dogClass.isFinal());

// Test regular class
Class catClass = Class::forName("Cat");
print("Cat is abstract: " + catClass.isAbstract());
print("Cat is final: " + catClass.isFinal());

print("Test passed");
