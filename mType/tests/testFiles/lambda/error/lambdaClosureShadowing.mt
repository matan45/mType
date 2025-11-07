// Local variable shadowing captured variables test
interface Function {
    function apply(int x) : int;
}

print("=== Closure Shadowing Test ===");

int outer = 10;

Function shadowTest = x -> {
    int outer = 20;  // Shadows the outer variable
    return outer + x;
};

print("Result: " + shadowTest.apply(5));  // Should use local outer = 20
print("Outer unchanged: " + outer);       // Should still be 10

// Nested shadowing
int value = 100;
Function nestedShadow = a -> {
    int value = 200;
    Function inner = b -> {
        int value = 300;
        return value + b;  // Uses innermost value = 300
    };
    return inner.apply(a) + value;  // Uses middle value = 200
};

print("Nested shadow: " + nestedShadow.apply(1));  // 300+1 + 200 = 501
print("Original value: " + value);  // Still 100

print("Shadowing complete");
