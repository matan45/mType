// Test: Private constructor access violation from external context
class Restricted {
    public int value;

    private constructor() {
        value = 100;
    }

    public static function create():  Restricted {
        return new Restricted();
    }
}

Restricted r = new Restricted();  // ERROR: Cannot access private constructor
