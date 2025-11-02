// Test: Mutual recursion between methods
// Expected: Pass - demonstrates mutually recursive methods

class EvenOdd {
    // Mutually recursive methods
    public bool isEven(int n) {
        print("isEven(" + n + ")");
        if (n == 0) {
            return true;
        }
        if (n < 0) {
            return this.isEven(-n);
        }
        return this.isOdd(n - 1);
    }

    public bool isOdd(int n) {
        print("isOdd(" + n + ")");
        if (n == 0) {
            return false;
        }
        if (n < 0) {
            return this.isOdd(-n);
        }
        return this.isEven(n - 1);
    }
}

EvenOdd checker = new EvenOdd();

print("Test 1: Is 4 even?");
bool result1 = checker.isEven(4);
print("Result: " + result1);

print("\nTest 2: Is 5 even?");
bool result2 = checker.isEven(5);
print("Result: " + result2);

print("\nTest 3: Is 3 odd?");
bool result3 = checker.isOdd(3);
print("Result: " + result3);

print("\nTest 4: Is 6 odd?");
bool result4 = checker.isOdd(6);
print("Result: " + result4);
