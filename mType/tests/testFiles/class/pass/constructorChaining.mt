// Test: Constructor chaining - super() calls parent constructor
// Expected: Pass - demonstrates constructor chaining with super()

class Vehicle {
    String type;
    int wheels;

    constructor(String type, int wheels) {
        this.type = type;
        this.wheels = wheels;
        print("Vehicle constructor: " + type + " with " + wheels + " wheels");
    }
}

class Car extends Vehicle {
    String brand;

    constructor(String brand) {
        super("Car", 4);  // Call parent constructor
        this.brand = brand;
        print("Car constructor: " + brand);
    }
}

class SportsCar extends Car {
    int topSpeed;

    constructor(String brand, int topSpeed) {
        super(brand);  // Call Car constructor, which calls Vehicle constructor
        this.topSpeed = topSpeed;
        print("SportsCar constructor: Top speed " + topSpeed + " mph");
    }
}

// Test constructor chaining
print("Creating a SportsCar:");
SportsCar ferrari = new SportsCar("Ferrari", 211);
print("Brand: " + ferrari.brand);
print("Type: " + ferrari.type);
print("Wheels: " + ferrari.wheels);
print("Top Speed: " + ferrari.topSpeed);
