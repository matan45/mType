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

Dog myDog = getDog();  // Valid
print("Function returning object test passed");