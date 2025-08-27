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

Car car = new Car("Toyota");
Bike bike = new Bike();
car = bike;  // Should throw type mismatch error