// Test: Using a nullable parameter without null check should fail at compile time

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function getBoxValue(Box? b): int {
    return b.value;
}

Box box = new Box(5);
print(getBoxValue(box));
print("Should not reach here");
