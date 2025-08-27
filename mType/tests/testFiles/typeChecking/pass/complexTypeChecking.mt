// Test with inheritance scenario (if supported later)
class Animal {
    string name;
    constructor(string n) {
        name = n;
    }
}

class Dog {
    string breed;
    constructor(string b) {
        breed = b;
    }
}

Animal a = new Animal("Generic");
Dog d = new Dog("Labrador");

// These should fail
// a = d;  // Different types