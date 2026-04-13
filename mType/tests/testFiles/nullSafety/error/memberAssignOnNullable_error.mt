// Test: Assigning a field on a nullable receiver should fail at compile time

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

Box? b = new Box(10);
b.value = 20;
print("Should not reach here");
