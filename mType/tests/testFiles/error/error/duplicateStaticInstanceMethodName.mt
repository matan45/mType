// Pass: Static and instance method with same name are allowed

class Counter {
    int count;

    constructor() {
        this.count = 0;
    }

    // Instance method
    public function reset(): void {
        this.count = 0;
        print("Instance reset called");
    }

    // Static method with same name - this is ALLOWED
    public static function reset(): void {
        print("Static reset called");
    }
}

// Call both methods
Counter c = new Counter();
c.reset();              // Calls instance method
Counter::reset();        // Calls static method

print("Both methods work correctly");
