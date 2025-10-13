import * from "test_library.mt";



// Test static method calls before creating any objects
print("=== Static Methods Before Object Creation ===");
print("Version: ");
print(Calculator::getVersion());
print("Initial operation count: ");
print(Calculator::getOperationCount());

// Reset counter to start fresh
Calculator::resetCounter();
print("Reset operation count: ");
print(Calculator::getOperationCount());

print("\n=== Object Creation and Method Calls ===");

// Create objects using different constructors
Calculator calc1 = new Calculator();
print("Created calc1 with default constructor");
print("calc1 name: ");
print(calc1.getName());

Calculator calc2 = new Calculator("Advanced Calculator", true);
print("Created calc2 with parameters");
print("calc2 name: ");
print(calc2.getName());

// Test instance methods
print("\n=== Instance Method Tests ===");
int result1 = calc1.add(15, 25);
print("calc1.add(15, 25) = ");
print(result1);

int result2 = calc2.multiply(6, 7);
print("calc2.multiply(6, 7) = ");
print(result2);

// Test static method to check operation count
print("Current operation count: ");
print(Calculator::getOperationCount());

// Test utility functions from the imported file
print("\n=== Utility Function Tests ===");
Calculator calc3 = createDefaultCalculator();
print("Created calc3 using utility function");
print("calc3 name: ");
print(calc3.getName());

Calculator calc4 = createNamedCalculator("Scientific Calculator");
print("Created calc4 using named utility function");
print("calc4 name: ");
print(calc4.getName());

// Test deactivating a calculator
print("\n=== State Modification Tests ===");
calc2.setActive(false);
int result3 = calc2.add(10, 20);
print("calc2.add(10, 20) after deactivation = ");
print(result3);

// Final operation count
print("Final operation count: ");
print(Calculator::getOperationCount());

print("Version: ");
print(Calculator::getVersion());