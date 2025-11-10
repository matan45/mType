// What happens when captured objects change test
interface Function {
    function apply(int x) : int;
}

class MutableBox {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue() : int {
        return this.value;
    }

    public function setValue(int v) {
        this.value = v;
    }
}

print("=== Closure Modification Test ===");

MutableBox box = new MutableBox(10);
Function reader = x -> box.getValue() + x;

print("Initial box value: " + box.getValue());
print("reader(5) = " + reader.apply(5));

// Modify the captured object
box.setValue(20);
print("After setting box to 20:");
print("reader(5) = " + reader.apply(5));

box.setValue(100);
print("After setting box to 100:");
print("reader(5) = " + reader.apply(5));

// Array modification test
int[] data = [1, 2, 3];
Function arraySum = dummy -> {
    int sum = 0;
    for (int i = 0; i < data.length; i = i + 1) {
        sum = sum + data[i];
    }
    return sum;
};

print("Initial array sum: " + arraySum.apply(0));

data[0] = 10;
data[1] = 20;
data[2] = 30;

print("After modifying array:");
print("Array sum: " + arraySum.apply(0));

print("Closure modification complete");
