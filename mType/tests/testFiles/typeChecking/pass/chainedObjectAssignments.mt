class Vehicle {
    public string brand;
    constructor(string b) {
        brand = b;
    }
}

class Car {
    public string brand;
    public int doors;
    constructor(string b, int d) {
        brand = b;
        doors = d;
    }
}

// Test basic chained assignment
Vehicle v1 = new Vehicle("Toyota");
Vehicle v2 = new Vehicle("Honda");
Vehicle v3 = new Vehicle("Ford");
Vehicle v4 = new Vehicle("BMW");

// Chained assignments: a = b = c = d
v1 = v2 = v3 = v4;
print("Chained assignment test 1: " + v1.brand);
print("Chained assignment test 2: " + v2.brand);
print("Chained assignment test 3: " + v3.brand);

// Test with same type objects
Car c1 = new Car("Mercedes", 4);
Car c2 = new Car("Audi", 2);

// Chain with same type
c1 = c2;
print("Car assignment: " + c1.brand);

// Test circular assignment pattern
Vehicle a = new Vehicle("A");
Vehicle b = new Vehicle("B");
Vehicle c = new Vehicle("C");

Vehicle temp = a;
a = b;
b = c;
c = temp;

print("Circular assignment A: " + a.brand);
print("Circular assignment B: " + b.brand);
print("Circular assignment C: " + c.brand);