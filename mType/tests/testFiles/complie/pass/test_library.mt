// Comprehensive class library for testing serialization
class Calculator {
    // Static members
    static int operationCount = 0;
    static final string VERSION = "1.0.0";

    // Instance fields
    string name;
    bool isActive;

    // Default constructor
    constructor() {
        name = "Default Calculator";
        isActive = true;
        Calculator::operationCount = Calculator::operationCount + 1;
    }

    // Parameterized constructor
    constructor(string calcName, bool active) {
        name = calcName;
        isActive = active;
        Calculator::operationCount = Calculator::operationCount + 1;
    }

    // Instance methods
    function add(int a, int b): int {
        if (isActive) {
            Calculator::operationCount = Calculator::operationCount + 1;
            return a + b;
        }
        return 0;
    }

    function multiply(int x, int y): int {
        if (isActive) {
            Calculator::operationCount = Calculator::operationCount + 1;
            return x * y;
        }
        return 0;
    }

    function getName(): string {
        return name;
    }

    function setActive(bool active): void {
        isActive = active;
    }

    // Static methods
    static function getOperationCount(): int {
        return Calculator::operationCount;
    }

    static function getVersion(): string {
        return Calculator::VERSION;
    }

    static function resetCounter(): void {
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