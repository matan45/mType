// Test: Smart cast (null narrowing) after null check

class Data {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Data? maybeData = new Data(42);

// After null check, variable should be treated as non-null
if (maybeData != null) {
    print("Value: " + maybeData.getValue());
}

// Test with null value
Data? nullData = null;
if (nullData != null) {
    print("Should not reach here");
} else {
    print("nullData is null as expected");
}

// Test reverse pattern (== null)
Data? another = new Data(100);
if (another == null) {
    print("Should not reach here");
} else {
    print("another value: " + another.getValue());
}

print("Smart cast tests passed!");
