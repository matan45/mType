// Test: Cast and access members
class Vehicle {
    public int wheels;

    public constructor(int w) {
        this.wheels = w;
    }
}

class Car extends Vehicle {
    public string model;

    public constructor(int w, string m):super(w) {
        this.model = m;
    }
}

Car car = new Car(4, "Tesla");
Vehicle vehicle = (Vehicle)car;
print(vehicle.wheels);  // Expected: 4

Car castedCar = (Car)vehicle;
print(castedCar.model);  // Expected: Tesla
print(castedCar.wheels);  // Expected: 4

// Expected output:
// 4
// Tesla
// 4
