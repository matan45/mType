// MYT-378: a value-class instance widened into an Object[16]. A value-class
// box is not the exact element class (Object), so canStoreExact() rejects it
// and it stores via the heterogeneous fallback; casting back must recover the
// box and its fields intact.
print("Testing value class in Object array");

value class Point {
    private int x;
    private int y;
    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
    public function sum(): int {
        return this.x + this.y;
    }
}

Object[] slots = new Object[16];
slots[0] = new Point(3, 4);
slots[1] = new Point(10, 20);

Point p0 = (Point)slots[0];
print(p0.sum());

Point p1 = (Point)slots[1];
print(p1.sum());

print("Done");

// Expected output:
// Testing value class in Object array
// 7
// 30
// Done
