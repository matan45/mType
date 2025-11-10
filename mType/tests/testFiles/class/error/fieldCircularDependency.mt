// Test: Circular field dependency detection
// Expected: Error - circular dependency in field initialization

class CircularA {
    public static CircularB b = new CircularB();
    public int value = 10;
}

class CircularB {
    public static CircularA a = new CircularA();
    public int value = 20;
}

// This should cause an error due to circular dependency
print("Creating CircularA");
CircularA obj = new CircularA();
print(obj.value);
