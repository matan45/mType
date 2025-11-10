// Test: Casting in for loop
class Container {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

class SmartContainer extends Container {
    public int multiplier;

    constructor(int v, int m):super(v) {
        this.multiplier = m;
    }

    public function getScaled(): int {
        return this.value * this.multiplier;
    }
}

// Cast in for loop initialization
Container c = new SmartContainer(2, 3);
for (int i = 0; i < ((SmartContainer)c).getScaled(); i = i + 1) {
    print(i);
}

print("---");

// Cast in for loop condition
Container[] arr = new Container[3];
arr[0] = new SmartContainer(1, 2);
arr[1] = new SmartContainer(2, 2);
arr[2] = new SmartContainer(3, 2);

for (int i = 0; i < ((SmartContainer)arr[1]).getScaled()-1; i = i++) {
    print(((SmartContainer)arr[i]).getScaled());
}

// Expected output:
// 0
// 1
// 2
// 3
// 4
// 5
// ---
// 2
// 4
// 6
