// Test: Casting with fully qualified type names
// Expected: Pass - demonstrates casting using fully qualified type names from imported modules
@Script

import { Vehicle, Car, SportsCar } from "./modules/BaseTypes.mt";

// Test 1: Downcast using fully qualified type name
print("Test 1: Downcast with fully qualified type");
types.Vehicle v1 = new types.SportsCar("Ferrari", 220);
v1.start();

if (v1 instanceof types.SportsCar) {
    types.SportsCar sc = (types.SportsCar)v1;
    print("Top speed: " + sc.getTopSpeed() + " mph");
    print("Doors: " + sc.getDoors());
}

// Test 2: Upcast chain with fully qualified types
print("\nTest 2: Multiple level casting");
types.SportsCar sc2 = new types.SportsCar("Lamborghini", 215);
types.Car c2 = (types.Car)sc2;  // Upcast to Car
types.Vehicle v2 = (types.Vehicle)c2;  // Upcast to Vehicle
v2.start();

// Test 3: Downcast from Vehicle to SportsCar directly
print("\nTest 3: Direct downcast from base to derived");
types.Vehicle v3 = new types.SportsCar("Porsche", 200);
if (v3 instanceof types.SportsCar) {
    types.SportsCar sc3 = (types.SportsCar)v3;
    print("Brand: " + sc3.getBrand());
    print("Top speed: " + sc3.getTopSpeed() + " mph");
}

// Test 4: Cast in array context
print("\nTest 4: Casting with array elements");
types.Vehicle[] vehicles = new types.Vehicle[3];
vehicles[0] = new types.Vehicle("Generic");
vehicles[1] = new types.Car("Toyota", 4);
vehicles[2] = new types.SportsCar("McLaren", 230);

int i = 0;
while (i < 3) {
    print("Vehicle " + i + ":");
    if (vehicles[i] instanceof types.SportsCar) {
        types.SportsCar sc = (types.SportsCar)vehicles[i];
        print("  SportsCar - Top speed: " + sc.getTopSpeed());
    } else if (vehicles[i] instanceof types.Car) {
        types.Car c = (types.Car)vehicles[i];
        print("  Car - Doors: " + c.getDoors());
    } else {
        print("  Vehicle - Brand: " + vehicles[i].getBrand());
    }
    i = i + 1;
}
