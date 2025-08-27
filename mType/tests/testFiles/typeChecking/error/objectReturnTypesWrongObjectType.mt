class Dog {
    string breed;
    
    constructor(string b) {
        breed = b;
    }
}

class Cat {
    string color;
    
    constructor(string c) {
        color = c;
    }
}

function getDog(): Dog {
    return new Dog("Labrador");
}

Cat wrongAnimal = getDog();  // Should fail