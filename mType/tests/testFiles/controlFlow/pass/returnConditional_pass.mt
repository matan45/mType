// Test ternary operator in return statements
function abs(int x): int {
    return x < 0 ? -x : x;
}

function max(int a, int b): int {
    return a > b ? a : b;
}

function min(int a, int b): int {
    return a < b ? a : b;
}

function clamp(int value, int minVal, int maxVal): int {
    return value < minVal ? minVal : (value > maxVal ? maxVal : value);
}

function getStatus(int age): string {
    return age < 13 ? "child" : (age < 20 ? "teenager" : (age < 65 ? "adult" : "senior"));
}

function calculatePrice(int quantity, float basePrice): float {
    return quantity * (quantity > 10 ? basePrice * 0.9 : (quantity > 5 ? basePrice * 0.95 : basePrice));
}

function isLeapYear(int year): bool {
    return (year % 4 == 0) ? ((year % 100 == 0) ? (year % 400 == 0) : true) : false;
}

function sign(int n): int {
    return n > 0 ? 1 : (n < 0 ? -1 : 0);
}

function oddOrEven(int n): string {
    return n % 2 == 0 ? "even" : "odd";
}

print(abs(-15));              // 15
print(abs(20));               // 20
print(max(10, 25));           // 25
print(min(10, 25));           // 10
print(clamp(15, 0, 10));      // 10
print(clamp(-5, 0, 10));      // 0
print(clamp(5, 0, 10));       // 5
print(getStatus(10));         // child
print(getStatus(16));         // teenager
print(getStatus(30));         // adult
print(getStatus(70));         // senior
print(calculatePrice(3, 10.0));   // 30.0
print(calculatePrice(7, 10.0));   // 66.5
print(calculatePrice(12, 10.0));  // 108.0
print(isLeapYear(2000));      // true
print(isLeapYear(1900));      // false
print(isLeapYear(2024));      // true
print(sign(42));              // 1
print(sign(-8));              // -1
print(sign(0));               // 0
print(oddOrEven(10));         // even
print(oddOrEven(7));          // odd

print("Test passed"); // Test completed
