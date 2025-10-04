// Test: Private constructor with factory pattern
class Singleton {
    private static Singleton instance = null;
    public int value;

    private constructor() {
        value = 42;
    }

    public static function getInstance(): Singleton {
        if (instance == null) {
            instance = new Singleton();  // Private constructor accessible in same class
        }
        return instance;
    }
}

Singleton s1 = Singleton::getInstance();
print(s1.value);  // Expected: 42

Singleton s2 = Singleton::getInstance();
print(s2.value);  // Expected: 42
