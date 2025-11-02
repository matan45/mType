// Test 2D array element type casting
// Demonstrates casting elements within multi-dimensional arrays

class Vehicle {
    string type;
}

class Car extends Vehicle {
    int doors;

    Car() {
        type = "Car";
        doors = 4;
    }
}

class Truck extends Vehicle {
    int capacity;

    Truck() {
        type = "Truck";
        capacity = 1000;
    }
}

@Script
void test2DArrayElementCasting() {
    print("Testing 2D array element type casting");

    // Create 2D array of specific type
    Car[][] carMatrix = new Car[2][2];

    // Initialize with Car instances
    carMatrix[0][0] = new Car();
    carMatrix[0][1] = new Car();
    carMatrix[1][0] = new Car();
    carMatrix[1][1] = new Car();

    carMatrix[0][0].doors = 2;
    carMatrix[0][1].doors = 4;

    print("2D array dimensions: " + carMatrix.length + "x" + carMatrix[0].length);

    // Access through Vehicle reference
    Vehicle[][] vehicleMatrix = carMatrix;
    print("Vehicle type at [0][0]: " + vehicleMatrix[0][0].type);
    print("Vehicle type at [0][1]: " + vehicleMatrix[0][1].type);

    // Cast back individual elements
    Car car = (Car)vehicleMatrix[0][0];
    print("Car doors at [0][0]: " + car.doors);

    car = (Car)vehicleMatrix[0][1];
    print("Car doors at [0][1]: " + car.doors);

    print("2D array element casting completed");
}
