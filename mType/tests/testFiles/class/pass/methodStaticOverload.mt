// Test: Static method overloading
// Expected: Pass - demonstrates static method overloading

class Utils {
    // Static method overload 1
    public static int max(int a, int b) {
        print("max(int, int): " + a + ", " + b);
        if (a > b) {
            return a;
        }
        return b;
    }

    // Static method overload 2
    public static int max(int a, int b, int c) {
        print("max(int, int, int): " + a + ", " + b + ", " + c);
        int temp = Utils.max(a, b);
        return Utils.max(temp, c);
    }

    // Static method overload 3
    public static string max(string a, string b) {
        print("max(string, string): " + a + ", " + b);
        return a + b; // Simplified comparison
    }
}

// Test static method overloading
print("Test 1: Two ints");
int r1 = Utils.max(10, 5);
print("Result: " + r1);

print("\nTest 2: Three ints");
int r2 = Utils.max(10, 25, 15);
print("Result: " + r2);

print("\nTest 3: Two strings");
string r3 = Utils.max("Hello", "World");
print("Result: " + r3);
