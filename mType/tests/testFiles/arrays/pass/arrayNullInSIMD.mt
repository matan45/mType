// Test null checks in SIMD operations
print("Testing null checks in SIMD operations");

class Item {
    int value;

    Item(int v) {
        value = v;
    }

    int getValue() {
        return value;
    }
}

// Create array with SIMD threshold (16 elements)
Item[] items = new Item[16];
print("Created Item array with length: " + items.length);

// Initialize first half with objects, second half with null
for (int i = 0; i < 8; i++) {
    items[i] = new Item(i * 10);
}
for (int i = 8; i < 16; i++) {
    items[i] = null;
}

// Count null and non-null elements
int nullCount = 0;
int nonNullCount = 0;

for (int i = 0; i < items.length; i++) {
    if (items[i] == null) {
        nullCount = nullCount + 1;
    } else {
        nonNullCount = nonNullCount + 1;
    }
}

print("Null count: " + nullCount);
print("Non-null count: " + nonNullCount);

// Sum values of non-null elements
int sum = 0;
for (int i = 0; i < items.length; i++) {
    if (items[i] != null) {
        sum = sum + items[i].getValue();
    }
}

print("Sum of non-null values: " + sum);

print("Null checks in SIMD operations test completed");
