// Test: Circular instance field dependency detection
// Expected: Error - circular dependency in instance field initialization

class CircularA {
    public CircularB b = new CircularB();  // instance field
    public int value = 10;
}

class CircularB {
    public CircularA a = new CircularA();  // instance field
    public int value = 20;
}

// This should cause an error due to circular instance field initialization
print("Creating CircularA");
CircularA obj = new CircularA();
print(obj.value);
