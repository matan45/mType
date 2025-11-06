// Integration Test: Casting inside various loop constructs
// Combines casting with for, while, and nested loops

class Vehicle {
    public Int wheels;

    public constructor(Int w) {
        this.wheels = w;
    }

    public String getType() {
        return "Vehicle";
    }
}

class Car extends Vehicle {
    public String brand;

    public constructor(Int w, String b) {
        super(w);
        this.brand = b;
    }

    public String getType() {
        return "Car";
    }

    public String getBrand() {
        return this.brand;
    }
}

class Truck extends Vehicle {
    public Int cargo;

    public constructor(Int w, Int c) {
        super(w);
        this.cargo = c;
    }

    public String getType() {
        return "Truck";
    }

    public Int getCargo() {
        return this.cargo;
    }
}

// Test 1: Casting in for loop
print("Test 1: Casting in for loop");
Vehicle vehicles[] = new Vehicle[3];
vehicles[0] = new Car(4, "Toyota");
vehicles[1] = new Truck(6, 1000);
vehicles[2] = new Car(4, "Honda");

for (Int i = 0; i < 3; i = i + 1) {
    if (vehicles[i] isClassOf Car) {
        print("Car brand: " + ((Car)vehicles[i]).getBrand());
    } else if (vehicles[i] isClassOf Truck) {
        print("Truck cargo: " + ((Truck)vehicles[i]).getCargo());
    }
}

// Test 2: Casting in while loop with dynamic condition
print("Test 2: Casting in while loop");
Int index = 0;
while (index < 3) {
    Vehicle v = vehicles[index];
    if (v isClassOf Car && ((Car)v).getBrand() == "Toyota") {
        print("Found Toyota at index " + index);
        index = 10; // Exit condition
    } else {
        index = index + 1;
    }
}

// Test 3: Nested loops with casting
print("Test 3: Nested loops with casting");
Vehicle fleet[][] = new Vehicle[2][2];
fleet[0][0] = new Car(4, "BMW");
fleet[0][1] = new Truck(8, 2000);
fleet[1][0] = new Car(4, "Audi");
fleet[1][1] = new Truck(10, 3000);

for (Int row = 0; row < 2; row = row + 1) {
    for (Int col = 0; col < 2; col = col + 1) {
        if (fleet[row][col] isClassOf Car) {
            Car c = (Car)fleet[row][col];
            print("Fleet[" + row + "][" + col + "] Car: " + c.getBrand());
        } else if (fleet[row][col] isClassOf Truck) {
            Truck t = (Truck)fleet[row][col];
            print("Fleet[" + row + "][" + col + "] Truck cargo: " + t.getCargo());
        }
    }
}

// Test 4: Casting in loop condition
print("Test 4: Casting in loop condition");
Vehicle testVehicle = new Car(4, "Test");
for (Int i = 0; i < ((Car)testVehicle).getBrand().length(); i = i + 1) {
    print("Character at index " + i);
}

// Test 5: Casting with break and continue
print("Test 5: Casting with break and continue");
for (Int i = 0; i < 3; i = i + 1) {
    if (vehicles[i] isClassOf Truck) {
        if (((Truck)vehicles[i]).getCargo() < 500) {
            print("Skipping small truck");
            continue;
        }
        print("Processing large truck: " + ((Truck)vehicles[i]).getCargo());
    } else if (vehicles[i] isClassOf Car) {
        if (((Car)vehicles[i]).getBrand() == "Honda") {
            print("Found target brand: Honda");
            break;
        }
        print("Processing car: " + ((Car)vehicles[i]).getBrand());
    }
}

print("All loop casting tests completed");
