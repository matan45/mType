// Test: Constructor chaining - super() calls parent constructor
// Expected: Pass - demonstrates constructor chaining with super()

class Vehicle {
    public string type;
    public int wheels;

    public constructor(string type, int wheels) {
        this.type = type;
        this.wheels = wheels;
        print("Vehicle constructor: " + type + " with " + wheels + " wheels");
    }
}

class Car extends Vehicle {
    public string brand;

    public constructor(string brand) : super("Car", 4) {
        this.brand = brand;
        print("Car constructor: " + brand);
    }
}

class SportsCar extends Car {
    public int topSpeed;

    public constructor(string brand, int topSpeed) : super(brand) {
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
