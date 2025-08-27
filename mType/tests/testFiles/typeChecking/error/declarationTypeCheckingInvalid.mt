class Vehicle {
    string type;
    constructor(string t) {
        type = t;
    }
}

class Animal {
    string species;
    constructor(string s) {
        species = s;
    }
}

Vehicle car = new Animal("Dog");  // Should fail