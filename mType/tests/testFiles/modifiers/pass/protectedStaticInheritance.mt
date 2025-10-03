// Test: Protected static members accessible in subclasses
class Base {
    protected static int counter = 0;

    protected static void incrementCounter() {
        counter = counter + 1;
    }
}

class Derived extends Base {
    public static void increment() {
        // Accessing protected static field and method from parent
        incrementCounter();
    }

    public static int getCount() {
        return counter;
    }
}

Derived.increment();
Derived.increment();
print(Derived.getCount());  // Expected: 2
