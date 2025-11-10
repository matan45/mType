// Test return with value in constructor - should error
// Constructors should not return values

class BadClass {
    public int value;

    public constructor(int v) {
        value = v;
        // ERROR: Cannot return value from constructor
        return 42;
    }
}

BadClass obj = new BadClass(10);
print(obj.value);
