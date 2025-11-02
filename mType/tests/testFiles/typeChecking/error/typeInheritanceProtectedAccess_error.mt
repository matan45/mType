// Test: Protected member access violation from outside class hierarchy
// Expected: Error - protected members not accessible outside class hierarchy

class Vehicle {
    protected string engineType;
    protected int horsePower;

    public constructor(string engineType, int horsePower) {
        this.engineType = engineType;
        this.horsePower = horsePower;
    }

    protected void startEngine() {
        print("Starting " + this.engineType + " engine with " + this.horsePower + " HP");
    }

    public void drive() {
        this.startEngine();  // OK: internal call
        print("Vehicle is moving");
    }
}

class Car extends Vehicle {
    private int doors;

    public constructor(string engineType, int horsePower, int doors) : super(engineType, horsePower) {
        this.doors = doors;
    }

    public void honk() {
        // OK: accessing protected member in subclass
        print("Car with " + this.horsePower + " HP honking");
        this.startEngine();  // OK: protected method accessible in subclass
    }
}

// ERROR: Attempting to access protected members from outside the class hierarchy
function testProtectedAccess(): void {
    Car car = new Car("V6", 300, 4);

    // ERROR: Cannot access protected field from outside class hierarchy
    print("Engine type: " + car.engineType);

    // ERROR: Cannot access protected field from outside class hierarchy
    print("Horse power: " + car.horsePower);

    // ERROR: Cannot access protected method from outside class hierarchy
    car.startEngine();
}

testProtectedAccess();
