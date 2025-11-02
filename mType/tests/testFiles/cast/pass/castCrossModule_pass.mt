// Test: Cross-module type casting with complex scenarios
// Expected: Pass - demonstrates advanced cross-module casting with selective imports
@Script

import { Vehicle, Car } from "./modules/BaseTypes.mt";
import { ElectricCar, Truck } from "./modules/ExtendedTypes.mt";

// Test 1: Cast between types from different modules
print("Test 1: Cross-module inheritance casting");
types.Vehicle baseVehicle = new extended.ElectricCar("BMW i4", 4, 80);
baseVehicle.start();

if (baseVehicle instanceof extended.ElectricCar) {
    extended.ElectricCar elec = (extended.ElectricCar)baseVehicle;
    print("Battery: " + elec.getBatteryCapacity() + " kWh");
    print("Doors: " + elec.getDoors());
    print("Brand: " + elec.getBrand());
}

// Test 2: Cast through intermediate type
print("\nTest 2: Multi-level cross-module casting");
types.Car midLevelCar = new extended.ElectricCar("Audi e-tron", 4, 95);
midLevelCar.start();

types.Vehicle upcast = (types.Vehicle)midLevelCar;
print("After upcast brand: " + upcast.getBrand());

if (upcast instanceof types.Car) {
    types.Car downcast = (types.Car)upcast;
    print("After downcast doors: " + downcast.getDoors());
}

// Test 3: Selective import casting
print("\nTest 3: Casting with selective imports");
Vehicle v1 = new Car("Mercedes", 4);
Car c1 = (Car)v1;
print("Selective import cast - Brand: " + c1.getBrand());

// Test 4: Mix of aliased and fully qualified casts
print("\nTest 4: Mixed qualification styles");
types.Vehicle fqVehicle = new extended.Truck("Kenworth", 25);
Vehicle aliasVehicle = (Vehicle)fqVehicle;
print("Brand after mixed cast: " + aliasVehicle.getBrand());

if (fqVehicle instanceof extended.Truck) {
    extended.Truck truck = (extended.Truck)fqVehicle;
    print("Truck payload: " + truck.getPayload() + " tons");
}

// Test 5: Complex array scenario with cross-module types
print("\nTest 5: Cross-module array casting");
types.Vehicle[] vehicles = new types.Vehicle[5];
vehicles[0] = new types.Vehicle("Generic");
vehicles[1] = new types.Car("Subaru", 4);
vehicles[2] = new types.SportsCar("Corvette", 194);
vehicles[3] = new extended.ElectricCar("Lucid Air", 4, 118);
vehicles[4] = new extended.Truck("Ram", 5);

int i = 0;
while (i < 5) {
    print("Index " + i + ":");
    types.Vehicle v = vehicles[i];

    if (v instanceof extended.ElectricCar) {
        extended.ElectricCar ec = (extended.ElectricCar)v;
        print("  Type: ElectricCar");
        print("  Battery: " + ec.getBatteryCapacity() + " kWh");
    } else if (v instanceof extended.Truck) {
        extended.Truck t = (extended.Truck)v;
        print("  Type: Truck");
        print("  Payload: " + t.getPayload() + " tons");
    } else if (v instanceof types.SportsCar) {
        types.SportsCar sc = (types.SportsCar)v;
        print("  Type: SportsCar");
        print("  Speed: " + sc.getTopSpeed() + " mph");
    } else if (v instanceof types.Car) {
        types.Car c = (types.Car)v;
        print("  Type: Car");
        print("  Doors: " + c.getDoors());
    } else {
        print("  Type: Vehicle");
        print("  Brand: " + v.getBrand());
    }

    i = i + 1;
}

// Test 6: Function parameter casting across modules
print("\nTest 6: Function parameter cross-module casting");

function processVehicle(types.Vehicle v): void {
    if (v instanceof extended.ElectricCar) {
        extended.ElectricCar ec = (extended.ElectricCar)v;
        print("Processing ElectricCar: " + ec.getBrand() + " (" + ec.getBatteryCapacity() + " kWh)");
    } else if (v instanceof extended.Truck) {
        extended.Truck t = (extended.Truck)v;
        print("Processing Truck: " + t.getBrand() + " (" + t.getPayload() + " tons)");
    } else {
        print("Processing Vehicle: " + v.getBrand());
    }
}

processVehicle(new extended.ElectricCar("Porsche Taycan", 4, 93));
processVehicle(new extended.Truck("Peterbilt", 30));
processVehicle(new types.Car("Nissan", 4));
