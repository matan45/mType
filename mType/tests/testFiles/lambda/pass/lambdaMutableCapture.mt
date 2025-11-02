// Capturing and modifying mutable objects test
interface Function {
    function apply(int x) : void;
}

class Counter {
    int count;

    function init(int initial) {
        this.count = initial;
    }

    function increment() {
        this.count = this.count + 1;
    }

    function getValue() : int {
        return this.count;
    }

    function add(int n) {
        this.count = this.count + n;
    }
}

print("=== Mutable Capture Test ===");

Counter c = new Counter(0);
print("Initial count: " + c.getValue());

Function incrementer = x -> {
    for (int i = 0; i < x; i = i + 1) {
        c.increment();
    }
};

incrementer.apply(5);
print("After incrementer(5): " + c.getValue());

incrementer.apply(3);
print("After incrementer(3): " + c.getValue());

// Lambda modifying array
int[] arr = [1, 2, 3, 4, 5];
Function doubler = idx -> {
    if (idx >= 0 && idx < arr.length) {
        arr[idx] = arr[idx] * 2;
    }
};

print("Original array:");
for (int i = 0; i < arr.length; i = i + 1) {
    print(arr[i]);
}

doubler.apply(2);  // Double element at index 2
doubler.apply(4);  // Double element at index 4

print("After doubling indices 2 and 4:");
for (int i = 0; i < arr.length; i = i + 1) {
    print(arr[i]);
}

print("Mutable capture complete");
