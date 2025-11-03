// Error: Cannot cast array of parent type to array of child type
// Array variance rules prevent this to ensure type safety

class Animal {
    string name;
}

class Dog extends Animal {
    function bark(): void {
        print("Woof!");
    }
}

class Cat extends Animal {
    function meow(): void {
        print("Meow!");
    }
}


function testParentArrayToChildArray(): void {
    Animal[] animals = new Animal[3];
    animals[0] = new Dog();
    animals[1] = new Cat();
    animals[2] = new Dog();

    // Error: Cannot cast Animal[] to Dog[]
    // The array contains both Dogs and Cats
    Dog[] dogs = (Dog[])animals;
}
testParentArrayToChildArray();
