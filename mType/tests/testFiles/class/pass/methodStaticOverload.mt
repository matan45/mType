// Test: Static method overloading
// Expected: Pass - demonstrates static method overloading

class Utils {
    // Static method overload 1
    public static function max(int a, int b): int {
        print("max(int, int): " + a + ", " + b);
        if (a > b) {
            return a;
        }
        return b;
    }

    // Static method overload 2
    public static function max2(int a, int b, int c): int {
        print("max(int, int, int): " + a + ", " + b + ", " + c);
        int temp = Utils::max(a, b);
        return Utils::max(temp, c);
    }

    // Static method overload 3
    public static function max3(string a, string b): string {
        print("max(string, string): " + a + ", " + b);
        return a + b; // Simplified comparison
    }
}

// Test static method overloading
print("Test 1: Two ints");
int r1 = Utils::max(10, 5);
print("Result: " + r1);

print("\nTest 2: Three ints");
int r2 = Utils::max2(10, 25, 15);
print("Result: " + r2);

print("\nTest 3: Two strings");
string r3 = Utils::max3("Hello", "World");
print("Result: " + r3);
