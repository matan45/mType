// Test: Upcasting from child to parent type
// Expected: Pass - child instances can be assigned to parent type variables

class Vehicle {
    protected string make;

    public constructor(string make) {
        this.make = make;
    }

    public void drive() {
        print("Driving vehicle: " + this.make);
    }

    public string getMake() {
        return this.make;
    }
}

class Car extends Vehicle {
    private int doors;

    public constructor(string make, int doors) : super(make) {
        this.doors = doors;
    }

    public void drive() {
        print("Driving car: " + this.make + " with " + this.doors + " doors");
    }

    public int getDoors() {
        return this.doors;
    }
}

class Motorcycle extends Vehicle {
    private bool hasSidecar;

    public constructor(string make, bool hasSidecar) : super(make) {
        this.hasSidecar = hasSidecar;
    }

    public void drive() {
        print("Riding motorcycle: " + this.make);
    }

    public bool getHasSidecar() {
        return this.hasSidecar;
    }
}

// Test 1: Direct upcast assignment
print("Test 1: Direct upcast");
Car car = new Car("Toyota", 4);
Vehicle v1 = car;  // Upcast: Car to Vehicle
v1.drive();
print("Make: " + v1.getMake());

// Test 2: Upcast with Motorcycle
print("\nTest 2: Motorcycle upcast");
Motorcycle bike = new Motorcycle("Harley", false);
Vehicle v2 = bike;  // Upcast: Motorcycle to Vehicle
v2.drive();
print("Make: " + v2.getMake());

// Test 3: Upcast in array initialization
print("\nTest 3: Upcast in array");
Vehicle[] vehicles = new Vehicle[3];
vehicles[0] = new Car("Honda", 2);
vehicles[1] = new Motorcycle("Yamaha", false);
vehicles[2] = new Vehicle("Generic");

int i = 0;
while (i < 3) {
    print("Vehicle " + i + ":");
    vehicles[i].drive();
    i = i + 1;
}

// Test 4: Upcast in function parameter (implicit)
function processVehicle(Vehicle v): void {
    print("Processing: " + v.getMake());
}

print("\nTest 4: Function parameter upcast");
Car testCar = new Car("Ford", 4);
processVehicle(testCar);  // Implicit upcast

Motorcycle testBike = new Motorcycle("Ducati", false);
processVehicle(testBike);  // Implicit upcast

print("\nAll upcast tests completed");
