// Test: Casting between imported types from different modules
// Expected: Pass - demonstrates cross-module type casting with imports
@Script

import { Vehicle, Car, SportsCar } from "./modules/BaseTypes.mt";
import { ElectricCar, Truck } from "./modules/ExtendedTypes.mt";

// Test 1: Cast imported base type to derived type
print("Test 1: Downcast imported Vehicle to SportsCar");
Vehicle v1 = new SportsCar("Tesla Roadster", 250);
v1.start();

if (v1 instanceof SportsCar) {
    SportsCar sc = (SportsCar)v1;
    print("Top speed: " + sc.getTopSpeed() + " mph");
}

// Test 2: Cast to extended type from different module
print("\nTest 2: Cast to ElectricCar from extended module");
types.Vehicle v2 = new extended.ElectricCar("Rivian", 4, 135);
v2.start();

if (v2 instanceof extended.ElectricCar) {
    extended.ElectricCar ec = (extended.ElectricCar)v2;
    print("Battery: " + ec.getBatteryCapacity() + " kWh");
    print("Doors: " + ec.getDoors());
}

// Test 3: Cast from base to different extended types
print("\nTest 3: Multiple extended type casts");
Vehicle v3 = new extended.Truck("Volvo", 20);
v3.start();

if (v3 instanceof extended.Truck) {
    extended.Truck t = (extended.Truck)v3;
    print("Payload capacity: " + t.getPayload() + " tons");
}

// Test 4: Upcast from extended type to base type
print("\nTest 4: Upcast ElectricCar to Vehicle");
extended.ElectricCar ec2 = new extended.ElectricCar("Nissan Leaf", 4, 62);
Vehicle v4 = (Vehicle)ec2;  // Explicit upcast
print("Brand: " + v4.getBrand());
v4.start();

// Test 5: Array with mixed imported types
print("\nTest 5: Array with cross-module types");
Vehicle[] fleet = new Vehicle[4];
fleet[0] = new Car("Toyota", 4);
fleet[1] = new SportsCar("Ferrari", 220);
fleet[2] = new extended.ElectricCar("Tesla Model S", 4, 100);
fleet[3] = new extended.Truck("Ford F-150", 3);

int i = 0;
while (i < 4) {
    print("Vehicle " + i + ":");
    if (fleet[i] instanceof extended.ElectricCar) {
        extended.ElectricCar e = (extended.ElectricCar)fleet[i];
        print("  ElectricCar - Battery: " + e.getBatteryCapacity() + " kWh");
    } else if (fleet[i] instanceof extended.Truck) {
        extended.Truck tr = (extended.Truck)fleet[i];
        print("  Truck - Payload: " + tr.getPayload() + " tons");
    } else if (fleet[i] instanceof SportsCar) {
        SportsCar s = (SportsCar)fleet[i];
        print("  SportsCar - Speed: " + s.getTopSpeed() + " mph");
    } else if (fleet[i] instanceof Car) {
        Car c = (Car)fleet[i];
        print("  Car - Doors: " + c.getDoors());
    }
    i = i + 1;
}

// Test 6: Chain casting across modules
print("\nTest 6: Chain casting");
types.Vehicle v5 = new extended.ElectricCar("Polestar", 4, 78);
types.Car c5 = (types.Car)v5;
extended.ElectricCar ec5 = (extended.ElectricCar)c5;
print("Final battery capacity: " + ec5.getBatteryCapacity() + " kWh");
