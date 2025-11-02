// Test: Cannot overload methods by return type only
// Expected: Error - return type alone is not sufficient for overloading

class ReturnTypeOverload {
    // First method
    public int getValue() {
        return 42;
    }

    // This should cause an error - same signature, different return type
    public string getValue() {
        return "42";
    }
}

ReturnTypeOverload obj = new ReturnTypeOverload();
print(obj.getValue());
