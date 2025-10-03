// Test: Private constructor access violation from external context
class Restricted {
    public int value;

    private Restricted() {
        value = 100;
    }

    public static Restricted create() {
        return new Restricted();
    }
}

Restricted r = new Restricted();  // ERROR: Cannot access private constructor
