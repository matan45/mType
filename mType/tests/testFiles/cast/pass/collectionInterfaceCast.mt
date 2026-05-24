// Test: cast a user-defined collection to its interface, invoke interface methods
interface IBag {
    public function size(): int;
    public function describe(): void;
}

class MyBag implements IBag {
    public int count;
    public constructor(int c) { this.count = c; }
    public function size(): int { return this.count; }
    public function describe(): void { print("Bag holding " + this.count + " items"); }
}

MyBag bag = new MyBag(7);
IBag asInterface = (IBag)bag;
asInterface.describe();
print(asInterface.size());

// Expected output:
// Bag holding 7 items
// 7
