// Test: Casting operations in try-catch-finally blocks
// Expected: Pass - demonstrates casting in all exception handling blocks

import * from "../../lib/exceptions/Exception.mt";

class TypeException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class Vehicle {
    protected string model;

    public constructor(string model) {
        this.model = model;
    }

    public string getModel() {
        return this.model;
    }

    public void start() {
        print("Vehicle " + this.model + " starting");
    }
}

class Car extends Vehicle {
    private int doors;

    public constructor(string model, int doors) : super(model) {
        this.doors = doors;
    }

    public void start() {
        print("Car " + this.model + " engine starting");
    }

    public int getDoors() {
        return this.doors;
    }

    public void drive() {
        print("Driving car: " + this.model);
    }
}

class Truck extends Vehicle {
    private int capacity;

    public constructor(string model, int capacity) : super(model) {
        this.capacity = capacity;
    }

    public void start() {
        print("Truck " + this.model + " diesel starting");
    }

    public int getCapacity() {
        return this.capacity;
    }

    public void haul() {
        print("Hauling with truck: " + this.model);
    }
}

// Test 1: Cast in try block with catch and finally
function castInTryBlock(Vehicle v): string {
    try {
        print("Attempting cast in try block");
        if (v instanceof Car) {
            Car c = (Car)v;
            c.drive();
            return "Cast successful in try";
        } else {
            throw new TypeException("Not a car");
        }
    } catch (TypeException e) {
        print("Catch block: " + e.getMessage());
        if (v instanceof Truck) {
            Truck t = (Truck)v;
            t.haul();
            return "Cast successful in catch";
        }
        return "No valid cast";
    } finally {
        print("Finally: Cleanup for " + v.getModel());
    }
}

// Test 2: Cast in catch block
function castInCatchBlock(Vehicle v): string {
    try {
        throw new TypeException("Intentional error");
    } catch (TypeException e) {
        print("In catch: " + e.getMessage());
        if (v instanceof Car) {
            Car c = (Car)v;
            print("Cast to Car in catch: " + c.getDoors() + " doors");
            return "Car found";
        } else if (v instanceof Truck) {
            Truck t = (Truck)v;
            print("Cast to Truck in catch: " + t.getCapacity() + " tons");
            return "Truck found";
        }
        return "Unknown type";
    } finally {
        print("Finally executed");
    }
}

// Test 3: Cast in finally block
function castInFinallyBlock(Vehicle v1, Vehicle v2): string {
    string result = "not set";
    try {
        if (v1 instanceof Car) {
            Car c = (Car)v1;
            c.drive();
            result = "v1 processed";
        }
    } finally {
        print("Finally: Processing v2");
        if (v2 instanceof Truck) {
            Truck t = (Truck)v2;
            t.haul();
            print("v2 cast successful in finally");
        }
    }
    return result;
}

// Test 4: Multiple casts across try-catch-finally
function multipleCastsAllBlocks(Vehicle v1, Vehicle v2, Vehicle v3): string {
    int carCount = 0;
    int truckCount = 0;

    try {
        print("Try: Checking v1");
        if (v1 instanceof Car) {
            Car c = (Car)v1;
            carCount = carCount + 1;
            print("Found car: " + c.getModel());
        }
        throw new TypeException("Moving to catch");
    } catch (TypeException e) {
        print("Catch: Checking v2 - " + e.getMessage());
        if (v2 instanceof Truck) {
            Truck t = (Truck)v2;
            truckCount = truckCount + 1;
            print("Found truck: " + t.getModel());
        } else if (v2 instanceof Car) {
            Car c = (Car)v2;
            carCount = carCount + 1;
            print("Found car: " + c.getModel());
        }
    } finally {
        print("Finally: Checking v3");
        if (v3 instanceof Truck) {
            Truck t = (Truck)v3;
            truckCount = truckCount + 1;
            print("Found truck: " + t.getModel());
        } else if (v3 instanceof Car) {
            Car c = (Car)v3;
            carCount = carCount + 1;
            print("Found car: " + c.getModel());
        }
    }

    return "Cars: " + carCount + ", Trucks: " + truckCount;
}

// Test 5: Nested try-catch-finally with casts
function nestedCastBlocks(Vehicle v): string {
    try {
        print("Outer try");
        try {
            print("Inner try");
            if (v instanceof Car) {
                Car c = (Car)v;
                c.drive();
                throw new TypeException("After car operation");
            } else {
                throw new TypeException("Not a car");
            }
        } catch (TypeException e) {
            print("Inner catch: " + e.getMessage());
            if (v instanceof Truck) {
                Truck t = (Truck)v;
                t.haul();
                return "Truck processed in inner catch";
            }
            throw e;
        } finally {
            print("Inner finally");
        }
    } catch (TypeException e) {
        print("Outer catch: " + e.getMessage());
        return "Handled in outer catch";
    } finally {
        print("Outer finally");
    }
}

// Test 6: Cast with return in try-catch-finally
function castWithReturns(Vehicle v): string {
    try {
        if (v instanceof Car) {
            Car c = (Car)v;
            print("Returning from try: " + c.getModel());
            return "Car: " + c.getModel();
        }
        throw new TypeException("Not a car");
    } catch (TypeException e) {
        if (v instanceof Truck) {
            Truck t = (Truck)v;
            print("Returning from catch: " + t.getModel());
            return "Truck: " + t.getModel();
        }
        print("Returning default from catch");
        return "Unknown vehicle";
    } finally {
        print("Finally always executes");
    }
}

// Run all tests
print("=== Test 1: Cast in try block ===");
Vehicle car1 = new Car("Honda Civic", 4);
print(castInTryBlock(car1));

print("\n");
Vehicle truck1 = new Truck("Ford F-150", 2);
print(castInTryBlock(truck1));

print("\n=== Test 2: Cast in catch block ===");
Vehicle car2 = new Car("Toyota Camry", 4);
print(castInCatchBlock(car2));

print("\n");
Vehicle truck2 = new Truck("Chevy Silverado", 3);
print(castInCatchBlock(truck2));

print("\n=== Test 3: Cast in finally block ===");
Vehicle car3 = new Car("Mazda 3", 4);
Vehicle truck3 = new Truck("Ram 1500", 2);
print(castInFinallyBlock(car3, truck3));

print("\n=== Test 4: Multiple casts across all blocks ===");
Vehicle car4 = new Car("Nissan Altima", 4);
Vehicle truck4 = new Truck("GMC Sierra", 3);
Vehicle car5 = new Car("Hyundai Elantra", 4);
print(multipleCastsAllBlocks(car4, truck4, car5));

print("\n=== Test 5: Nested try-catch-finally with casts ===");
Vehicle car6 = new Car("Subaru Impreza", 4);
print(nestedCastBlocks(car6));

print("\n");
Vehicle truck5 = new Truck("Toyota Tundra", 2);
print(nestedCastBlocks(truck5));

print("\n=== Test 6: Cast with returns ===");
Vehicle car7 = new Car("Kia Forte", 4);
print(castWithReturns(car7));

print("\n");
Vehicle truck6 = new Truck("Dodge Ram", 3);
print(castWithReturns(truck6));

print("\nAll try-catch-finally cast tests completed");
