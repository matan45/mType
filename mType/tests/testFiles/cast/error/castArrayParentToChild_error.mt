// Error: Cannot cast array of parent type to array of child type
// Array variance rules prevent this to ensure type safety

class Animal {
    string name;
}

class Dog extends Animal {
    void bark() {
        print("Woof!");
    }
}

class Cat extends Animal {
    void meow() {
        print("Meow!");
    }
}

@Script
void testParentArrayToChildArray() {
    Animal[] animals = new Animal[3];
    animals[0] = new Dog();
    animals[1] = new Cat();
    animals[2] = new Dog();

    // Error: Cannot cast Animal[] to Dog[]
    // The array contains both Dogs and Cats
    Dog[] dogs = (Dog[])animals;
}
