// Test: Accessing a field on a nullable receiver should fail at compile time

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

Box? b = new Box(42);
print(b.value);
print("Should not reach here");
