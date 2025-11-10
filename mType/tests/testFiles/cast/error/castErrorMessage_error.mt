// Error: Test detailed error message for invalid cast
// This test verifies that the error message provides clear information
// about the incompatible types involved in the cast

class Animal {
    function speak() : String {
        return "Generic animal sound";
    }
}

class Dog extends Animal {
    function speak() : String {
        return "Woof!";
    }

    function fetch() : String {
        return "Fetching ball";
    }
}

class Cat extends Animal {
    function speak() : String {
        return "Meow!";
    }

    function scratch() : String {
        return "Scratching post";
    }
}

// Create a Dog instance
Dog myDog = new Dog();
print("Created dog: " + myDog.speak());

// Upcast to Animal (valid)
Animal animal = myDog;
print("Upcast to animal: " + animal.speak());

// Invalid downcast: trying to cast Dog (stored as Animal) to Cat
// Should produce a detailed error message indicating:
// - Attempted cast from Dog to Cat
// - These are sibling types, not in a valid inheritance relationship
Cat myCat = (Cat)animal;  // Error: Cannot cast Dog to Cat - incompatible types
