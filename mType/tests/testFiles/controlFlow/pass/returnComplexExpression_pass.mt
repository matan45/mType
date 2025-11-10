// Test large expressions as return values
function calculateDiscount(float price, int quantity): float {
    return price * quantity * (1.0 - (quantity > 10 ? 0.15 : (quantity > 5 ? 0.10 : 0.05)));
}

function computeScore(int base, int bonus, int penalty): int {
    return ((base * 2) + (bonus * 3)) - (penalty * 5);
}

function evaluateExpression(int a, int b, int c, int d): int {
    return (a + b) * (c - d) + (a * c) - (b * d);
}

function calculateArea(float radius): float {
    return 3.14159 * radius * radius;
}

function polynomialValue(int x): int {
    return (x * x * x) - (2 * x * x) + (3 * x) - 5;
}

function complexFormula(int x, int y, int z): int {
    return ((x + y * 2) * (z - 1)) / 3 + ((x * y) % z);
}

function fibonacci(int n): int {
    return n <= 1 ? n : fibonacci(n - 1) + fibonacci(n - 2);
}

print(calculateDiscount(100.0, 12));  // Approximately 1020.0 (100 * 12 * 0.85)
print(computeScore(10, 5, 2));        // 25 ((10*2) + (5*3)) - (2*5)
print(evaluateExpression(2, 3, 4, 5));  // 5 (2+3)*(4-5) + (2*4) - (3*5)
print(calculateArea(5.0));            // Approximately 78.53975
print(polynomialValue(3));            // 13 (27 - 18 + 9 - 5)
print(complexFormula(10, 5, 4));      // 42
print(fibonacci(8));                  // 21

print("Test passed"); // Test completed
