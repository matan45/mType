// Test generic array variance
print("Testing generic array variance");

class Box<T> {
    T value;

    Box(T v) {
        value = v;
    }

    T getValue() {
        return value;
    }
}

// Create array of generic boxes
Box<int>[] intBoxes = new Box<int>[3];
intBoxes[0] = new Box<int>(10);
intBoxes[1] = new Box<int>(20);
intBoxes[2] = new Box<int>(30);

print("Box<int> array:");
for (int i = 0; i < intBoxes.length; i++) {
    print("  intBoxes[" + i + "].value = " + intBoxes[i].getValue());
}

Box<string>[] stringBoxes = new Box<string>[2];
stringBoxes[0] = new Box<string>("Hello");
stringBoxes[1] = new Box<string>("World");

print("Box<string> array:");
for (int i = 0; i < stringBoxes.length; i++) {
    print("  stringBoxes[" + i + "].value = " + stringBoxes[i].getValue());
}

print("Generic array variance test completed");
