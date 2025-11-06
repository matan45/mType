// Comprehensive class library for testing serialization
class Calculator {
    // Static members
    public static int operationCount = 0;
    static final string VERSION = "1.0.0";

    // Instance fields
    string name;
    bool isActive;

    // Default constructor
    constructor() {
        name = "Default Calculator";
        isActive = true;
        operationCount++;
    }

    // Parameterized constructor
    constructor(string calcName, bool active) {
        name = calcName;
        isActive = active;
        operationCount++;
    }

    // Instance methods
    public function add(int a, int b): int {
        if (isActive) {
            operationCount++;
            return a + b;
        }
        return 0;
    }

    public function multiply(int x, int y): int {
        if (isActive) {
            operationCount++;
            return x * y;
        }
        return 0;
    }

    public function getName(): string {
        return name;
    }

    public function setActive(bool active): void {
        isActive = active;
    }

    // Static methods
    public static function getOperationCount(): int {
        return Calculator::operationCount;
    }

    public static function getVersion(): string {
        return Calculator::VERSION;
    }

    public static function resetCounter(): void {
        Calculator::operationCount = 0;
    }
}

// Utility functions
function createDefaultCalculator(): Calculator {
    return new Calculator();
}

function createNamedCalculator(string name): Calculator {
    return new Calculator(name, true);
}