// Test: Missing super() - Constructor doesn't call super() when parent has constructor
// Expected: Error - super() call is required when parent has constructor with parameters

class Vehicle {
    String type;
    int wheels;

    constructor(String type, int wheels) {
        this.type = type;
        this.wheels = wheels;
    }
}

class Car extends Vehicle {
    String brand;

    constructor(String brand) {
        // ERROR: Missing super() call to parent constructor
        this.brand = brand;
    }
}

// This should never execute due to missing super() error
Car myCar = new Car("Toyota");
