// Test captured variable type checking in lambda closures
interface IntSupplier {
    function get() : int;
}

interface StringSupplier {
    function get() : string;
}

print("=== Lambda Closure Type Checking Test ===");

// Capture int variable - type must match interface return type
int capturedInt = 42;
IntSupplier supplier1 = () -> capturedInt;
print("Captured int: " + supplier1.get());

// Capture string variable - type must match interface return type
string capturedString = "Hello";
StringSupplier supplier2 = () -> capturedString;
print("Captured string: " + supplier2.get());

// Capture and use in computation - types must be compatible
int base = 10;
int multiplier = 3;
IntSupplier supplier3 = () -> base * multiplier;
print("Computed from closure: " + supplier3.get());

// Modify captured variable and verify type consistency
int counter = 0;
interface Incrementer {
    function increment() : int;
}
Incrementer inc = () -> {
    counter = counter + 1;
    return counter;
};
print("First increment: " + inc.increment());
print("Second increment: " + inc.increment());

print("Closure type checking passed");
