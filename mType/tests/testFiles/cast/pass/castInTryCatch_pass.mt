// Test: Casting in try-catch block with error handling
// This test demonstrates proper error handling for casting operations

class Vehicle {
    function describe() : String {
        return "Generic vehicle";
    }
}

class Car extends Vehicle {
    int doors;

    constructor(int d) {
        doors = d;
    }

    function describe() : String {
        return "Car with " + doors + " doors";
    }

    function honk() : String {
        return "Beep beep!";
    }
}

class Motorcycle extends Vehicle {
    bool hasSidecar;

    constructor(bool sidecar) {
        hasSidecar = sidecar;
    }

    function describe() : String {
        if (hasSidecar) {
            return "Motorcycle with sidecar";
        }
        return "Motorcycle";
    }

    function rev() : String {
        return "Vroom vroom!";
    }
}

print("=== Safe Casting with Try-Catch ===");

// Create different vehicle types
Car myCar = new Car(4);
Motorcycle myBike = new Motorcycle(false);

// Store in array as base type
Vehicle[] vehicles = new Vehicle[2];
vehicles[0] = myCar;
vehicles[1] = myBike;

// Safe casting with error handling
int carCount = 0;
int bikeCount = 0;
int failedCasts = 0;

// Try to cast each vehicle to Car
int i = 0;
while (i < 2) {
    try {
        Car car = (Car)vehicles[i];
        print("Successfully cast to Car: " + car.describe());
        print("  Car honks: " + car.honk());
        carCount = carCount + 1;
    } catch (String error) {
        print("Cast to Car failed for vehicle " + i);
        failedCasts = failedCasts + 1;
    }
    i = i + 1;
}

print("");

// Try to cast each vehicle to Motorcycle
i = 0;
while (i < 2) {
    try {
        Motorcycle bike = (Motorcycle)vehicles[i];
        print("Successfully cast to Motorcycle: " + bike.describe());
        print("  Bike revs: " + bike.rev());
        bikeCount = bikeCount + 1;
    } catch (String error) {
        print("Cast to Motorcycle failed for vehicle " + i);
        failedCasts = failedCasts + 1;
    }
    i = i + 1;
}

print("");
print("Summary:");
print("  Cars found: " + carCount);
print("  Bikes found: " + bikeCount);
print("  Failed casts: " + failedCasts);

// Test with null values
print("");
print("=== Null Casting Test ===");
Vehicle nullVehicle = null;

try {
    Car nullCar = (Car)nullVehicle;
    print("Null cast to Car succeeded: " + (nullCar == null));
} catch (String error) {
    print("Null cast to Car failed: " + error);
}

// Test valid downcast
print("");
print("=== Valid Downcast Test ===");
Vehicle vehicleRef = new Car(2);
try {
    Car actualCar = (Car)vehicleRef;
    print("Valid downcast succeeded: " + actualCar.describe());
    print("  Honking: " + actualCar.honk());
} catch (String error) {
    print("Unexpected error: " + error);
}

print("");
print("Casting with try-catch complete");
