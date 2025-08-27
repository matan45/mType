// Incompatible class conversion boundary error test
// Tests boundary cases where unrelated classes cannot be converted

class Vehicle {
    string brand;
    constructor(string b) { brand = b; }
}

class Animal {
    string species;
    constructor(string s) { species = s; }
}

function expectsVehicle(Vehicle v): Vehicle { 
    return v; 
}

Animal dog = new Animal("Canine");

// This should fail: attempting to pass Animal as Vehicle
expectsVehicle(dog);  // Animal cannot be converted to Vehicle