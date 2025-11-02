// Test deep copy vs shallow copy behavior
print("Testing array deep copy vs shallow copy");

class Container {
    int value;

    Container(int v) {
        value = v;
    }

    int getValue() {
        return value;
    }

    void setValue(int v) {
        value = v;
    }
}

// Create original array
Container[] original = new Container[3];
original[0] = new Container(10);
original[1] = new Container(20);
original[2] = new Container(30);

print("Original array:");
for (int i = 0; i < original.length; i++) {
    print("  original[" + i + "] = " + original[i].getValue());
}

// Create copy array (reference copy)
Container[] shallowCopy = new Container[3];
for (int i = 0; i < original.length; i++) {
    shallowCopy[i] = original[i];
}

// Modify through shallow copy
shallowCopy[0].setValue(100);

print("After modifying through shallow copy:");
print("  original[0] = " + original[0].getValue());
print("  shallowCopy[0] = " + shallowCopy[0].getValue());

// Create deep copy
Container[] deepCopy = new Container[3];
for (int i = 0; i < original.length; i++) {
    deepCopy[i] = new Container(original[i].getValue());
}

// Modify through deep copy
deepCopy[1].setValue(200);

print("After modifying through deep copy:");
print("  original[1] = " + original[1].getValue());
print("  deepCopy[1] = " + deepCopy[1].getValue());

print("Deep copy vs shallow copy test completed");
