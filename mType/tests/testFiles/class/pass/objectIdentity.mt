// Test: Object identity vs equality
// Expected: Pass - demonstrates difference between == and equals

class Value {
    private int data;

    public constructor(int data) {
        this.data = data;
        print("Created Value(" + data + ")");
    }

    public function equals(Value other): bool {
        if (other == null) {
            return false;
        }
        Value v = other;
        return this.data == v.data;
    }

    public function getData(): int {
        return this.data;
    }
}

// Test identity vs equality
print("Test 1: Same reference");
Value v1 = new Value(42);
Value v2 = v1;
print("v1 == v2 (identity): " + (v1 == v2));
print("v1.equals(v2): " + v1.equals(v2));

print("\nTest 2: Equal values, different objects");
Value v3 = new Value(42);
Value v4 = new Value(42);
print("v3 == v4 (identity): " + (v3 == v4));
print("v3.equals(v4): " + v3.equals(v4));

print("\nTest 3: Different values");
Value v5 = new Value(10);
Value v6 = new Value(20);
print("v5 == v6 (identity): " + (v5 == v6));
print("v5.equals(v6): " + v5.equals(v6));

print("\nTest 4: Null comparison");
Value? v7 = null;
Value? v8 = null;
print("null == null: " + (v7 == v8));

print("\nTest 5: Array of objects");
Value[] arr1 = new Value[2];
arr1[0] = new Value(100);
arr1[1] = new Value(200);

Value[] arr2 = arr1;
print("arr1 == arr2 (same array): " + (arr1 == arr2));
print("arr1[0] == arr2[0] (same element): " + (arr1[0] == arr2[0]));

Value[] arr3 = new Value[2];
arr3[0] = new Value(100);
arr3[1] = new Value(200);
print("arr1 == arr3 (different arrays): " + (arr1 == arr3));
print("arr1[0].equals(arr3[0]) (equal values): " + arr1[0].equals(arr3[0]));
