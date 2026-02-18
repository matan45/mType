// Test: Value class structural equality
// Expected: Pass - == uses structural equality for value types

value class Pair {
    private int first;
    private int second;

    public constructor(int first, int second) {
        this.first = first;
        this.second = second;
    }

    public function getFirst(): int {
        return this.first;
    }

    public function getSecond(): int {
        return this.second;
    }

    public function toString(): string {
        return "(" + this.first + ", " + this.second + ")";
    }
}

// Same values should be equal
Pair p1 = new Pair(1, 2);
Pair p2 = new Pair(1, 2);
print("p1: " + p1.toString());
print("p2: " + p2.toString());
print("p1 == p2: " + (p1 == p2));
print("p1 != p2: " + (p1 != p2));

// Different values should not be equal
Pair p3 = new Pair(3, 4);
print("p3: " + p3.toString());
print("p1 == p3: " + (p1 == p3));
print("p1 != p3: " + (p1 != p3));

// Null comparisons
Pair p4 = null;
print("p4 == null: " + (p4 == null));
print("p1 == null: " + (p1 == null));
