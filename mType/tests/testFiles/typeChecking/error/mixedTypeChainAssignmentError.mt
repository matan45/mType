class Vehicle {
    string brand;
    constructor(string b) {
        brand = b;
    }
}

class Person {
    string name;
    constructor(string n) {
        name = n;
    }
}

class Building {
    int floors;
    constructor(int f) {
        floors = f;
    }
}

// Test mixed incompatible types in chain
Vehicle v1 = new Vehicle("Toyota");
Vehicle v2 = new Vehicle("Honda");
Person p1 = new Person("John");
Building b1 = new Building(10);

// This should fail - cannot mix Vehicle, Person, and Building in assignment chain
v1 = v2 = p1 = b1;