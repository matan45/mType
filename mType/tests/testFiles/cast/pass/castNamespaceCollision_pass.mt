// Test: Casting with namespace collision handling
// Expected: Pass - demonstrates casting when same class names exist in different namespaces
@Script

import { Vehicle as TypesVehicle, Car as TypesCar } from "./modules/BaseTypes.mt";
import { Vehicle as VehiclesVehicle, Car as VehiclesCar } from "./modules/AlternateTypes.mt";

// Test 1: Casting with types namespace (using fully qualified names)
print("Test 1: Cast with types namespace");
types.Vehicle tv1 = new types.Car("Honda", 4);
tv1.start();

if (tv1 instanceof types.Car) {
    types.Car tc1 = (types.Car)tv1;
    print("Brand: " + tc1.getBrand());
    print("Doors: " + tc1.getDoors());
}

// Test 2: Casting with vehicles namespace (using fully qualified names)
print("\nTest 2: Cast with vehicles namespace");
vehicles.Vehicle vv1 = new vehicles.Car("Sedan", "Blue");
print("Model: " + vv1.getModel());

if (vv1 instanceof vehicles.Car) {
    vehicles.Car vc1 = (vehicles.Car)vv1;
    print("Color: " + vc1.getColor());
}

// Test 3: Using aliased imports for casting
print("\nTest 3: Cast with aliased imports");
TypesVehicle tv2 = new TypesCar("Ford", 2);
tv2.start();

if (tv2 instanceof TypesCar) {
    TypesCar tc2 = (TypesCar)tv2;
    print("Doors on aliased car: " + tc2.getDoors());
}

// Test 4: Mix of fully qualified and aliased types
print("\nTest 4: Mixed namespace casting");
types.Vehicle tv3 = new types.SportsCar("Aston Martin", 210);
TypesVehicle tv4 = (TypesVehicle)tv3;  // Cast to alias
print("Brand via alias: " + tv4.getBrand());

// Test 5: Array with mixed namespace types
print("\nTest 5: Arrays with namespace-qualified casts");
types.Vehicle[] typesArray = new types.Vehicle[2];
typesArray[0] = new types.Car("Mazda", 4);
typesArray[1] = new types.SportsCar("Bugatti", 260);

int i = 0;
while (i < 2) {
    if (typesArray[i] instanceof types.SportsCar) {
        types.SportsCar sc = (types.SportsCar)typesArray[i];
        print("SportsCar top speed: " + sc.getTopSpeed());
    } else if (typesArray[i] instanceof types.Car) {
        types.Car c = (types.Car)typesArray[i];
        print("Car doors: " + c.getDoors());
    }
    i = i + 1;
}
