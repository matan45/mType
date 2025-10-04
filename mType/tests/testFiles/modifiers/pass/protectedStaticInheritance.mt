// Test: Protected static members accessible in subclasses
class Base {
    protected static int counter = 0;

    protected static function incrementCounter(): void {
        counter = counter + 1;
    }
}

class Derived extends Base {
    public static function increment(): void {
        // Accessing protected static field and method from parent
        Base::incrementCounter();
    }

    public static function getCount(): int {
        return counter;
    }
}

Derived::increment();
Derived::increment();
print(Derived::getCount());  // Expected: 2
