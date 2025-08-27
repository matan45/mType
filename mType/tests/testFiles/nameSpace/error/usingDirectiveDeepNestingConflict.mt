// Test error: type mismatch with using directive context

class Car {
    string model;
    constructor(string m) {
        model = m;
    }
}

class Bike {
    int wheels;
    constructor() {
        wheels = 2;
    }
}

namespace vehicles {
    Car defaultCar = new Car("Toyota");
    Bike defaultBike = new Bike();
}

using namespace vehicles;

// This should cause a type mismatch error
Bike wrongAssignment = defaultCar;  // ERROR: Cannot assign Car to Bike