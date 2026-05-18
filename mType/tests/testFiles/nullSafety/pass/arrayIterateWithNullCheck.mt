// Test: iterate an object array with a null check guarding access.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

Box[] arr = new Box[4];
arr[0] = new Box(10);
arr[2] = new Box(30);

for (int i = 0; i < arr.length; i = i + 1) {
    Box? item = arr[i];
    if (item != null) {
        print("[" + i + "] = " + item.getValue());
    } else {
        print("[" + i + "] = null");
    }
}
print("Array iterate-with-null-check tests passed!");
