// Test: copy an array element into a local nullable, then narrow on the
// local — the supported pattern for safe dispatch on array contents.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Box[] arr = new Box[3];
arr[0] = new Box(100);

Box? first = arr[0];
if (first != null) {
    print("first: " + first.getValue());
}

Box? second = arr[1];
if (second == null) {
    print("second is null");
}

Box? third = arr[2];
if (third != null) {
    print("should not reach here: " + third.getValue());
} else {
    print("third is null");
}

print("Local copy from array narrowing tests passed!");
