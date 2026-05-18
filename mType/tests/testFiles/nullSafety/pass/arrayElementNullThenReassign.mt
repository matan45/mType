// Test: an array element can be set to null and then reassigned to a
// fresh object — the slot survives null both ways.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Box[] arr = new Box[2];
arr[0] = new Box(5);

Box? before = arr[0];
if (before != null) {
    print("before: " + before.getValue());
}

arr[0] = null;
Box? mid = arr[0];
if (mid == null) {
    print("mid is null");
}

arr[0] = new Box(99);
Box? after = arr[0];
if (after != null) {
    print("after: " + after.getValue());
}

print("Array element null-then-reassign tests passed!");
