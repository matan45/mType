// Test: Casting in try-catch block with error handling
// This test demonstrates proper error handling for casting operations

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

class Vehicle {
    public function describe() : string {
        return "Generic vehicle";
    }
}

class Car extends Vehicle {
    public int doors;

    public constructor(int d) {
        this.doors = d;
    }

    public function describe() : string {
        return "Car with " + (string)this.doors + " doors";
    }

    public function honk() : string {
        return "Beep beep!";
    }
}

class Motorcycle extends Vehicle {
    public bool hasSidecar;

    public constructor(bool sidecar) {
        this.hasSidecar = sidecar;
    }

    public function describe() : string {
        if (this.hasSidecar) {
            return "Motorcycle with sidecar";
        }
        return "Motorcycle";
    }

    public function rev() : string {
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
    } catch (Exception e) {
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
    } catch (Exception e) {
        print("Cast to Motorcycle failed for vehicle " + (string)i);
        failedCasts = failedCasts + 1;
    }
    i = i + 1;
}

print("");
print("Summary:");
print("  Cars found: " + (string)carCount);
print("  Bikes found: " + (string)bikeCount);
print("  Failed casts: " + (string)failedCasts);

// Test with null values
print("");
print("=== Null Casting Test ===");
Vehicle nullVehicle = null;

try {
    Car nullCar = (Car)nullVehicle;
    if (nullCar == null) {
        print("Null cast to Car succeeded: true");
    } else {
        print("Null cast to Car succeeded: false");
    }
} catch (Exception e) {
    print("Null cast to Car failed: exception caught");
}

// Test valid downcast
print("");
print("=== Valid Downcast Test ===");
Vehicle vehicleRef = new Car(2);
try {
    Car actualCar = (Car)vehicleRef;
    print("Valid downcast succeeded: " + actualCar.describe());
    print("  Honking: " + actualCar.honk());
} catch (Exception e) {
    print("Unexpected error: exception caught");
}

print("");
print("Casting with try-catch complete");
