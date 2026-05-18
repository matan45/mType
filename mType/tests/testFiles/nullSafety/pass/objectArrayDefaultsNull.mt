// Test: `new Box[3]` leaves every slot as null, just like
// `new string[3]` does.

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

Box[] arr = new Box[3];
for (int i = 0; i < arr.length; i = i + 1) {
    if (arr[i] == null) {
        print("arr[" + i + "] = null");
    } else {
        print("arr[" + i + "] = not-null");
    }
}
print("Object array defaults-null tests passed!");
