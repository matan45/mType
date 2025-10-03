// Test: Private static members accessible within same class
class IDGenerator {
    private static int nextID = 1;

    private static int getNextID() {
        int current = nextID;
        nextID = nextID + 1;
        return current;
    }

    public static int generateID() {
        // Accessing private static field and method
        return getNextID();
    }
}

print(IDGenerator.generateID());  // Expected: 1
print(IDGenerator.generateID());  // Expected: 2
print(IDGenerator.generateID());  // Expected: 3
