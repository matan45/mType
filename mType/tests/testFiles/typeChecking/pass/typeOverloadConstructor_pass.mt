// Test constructor overload resolution
class Calculator {
    public int value;
    public string description;

    // Constructor with single int parameter
    constructor(int v) {
        value = v;
        description = "Integer calculator";
    }

    // Constructor with int and string parameters
    constructor(int v, string desc) {
        value = v;
        description = desc;
    }

    // Constructor with string parameter
    constructor(string desc) {
        value = 0;
        description = desc;
    }

    public function getInfo(): string {
        return description + ": " + value;
    }
}

function main(): void {
    print("Testing constructor overload resolution");

    // Call single int constructor
    Calculator calc1 = new Calculator(42);
    print(calc1.getInfo());

    // Call int + string constructor
    Calculator calc2 = new Calculator(100, "Custom calculator");
    print(calc2.getInfo());

    // Call string constructor
    Calculator calc3 = new Calculator("Default calculator");
    print(calc3.getInfo());

    print("Constructor overload test completed");
}

main();
