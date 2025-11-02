// Test: Invalid cast detection at runtime
// Expected: Error - casting to incompatible type

class Vehicle {
    public void move() {
        print("Vehicle moving");
    }
}

class Car extends Vehicle {
    public void honk() {
        print("Honk!");
    }
}

class Boat extends Vehicle {
    public void sail() {
        print("Sailing");
    }
}

// This should cause a runtime error
Vehicle v = new Car();
Boat b = (Boat)v;  // Invalid cast: Car cannot be cast to Boat
b.sail();
