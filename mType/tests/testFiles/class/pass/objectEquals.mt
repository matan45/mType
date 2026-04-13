// Test: equals() method override and comparison
// Expected: Pass - demonstrates custom equality

class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function equals(Point? other): bool {
        if (other == null) {
            return false;
        }
        Point otherPoint = other;
        return this.x == otherPoint.x && this.y == otherPoint.y;
    }

    public function getX(): int {
        return this.x;
    }

    public function getY(): int {
        return this.y;
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

// Test equals override
print("Test 1: Equal points");
Point p1 = new Point(5, 10);
Point p2 = new Point(5, 10);
print("p1: " + p1.toString());
print("p2: " + p2.toString());
print("p1.equals(p2): " + p1.equals(p2));
print("p1 == p2: " + (p1 == p2));

print("\nTest 2: Different points");
Point p3 = new Point(3, 7);
print("p3: " + p3.toString());
print("p1.equals(p3): " + p1.equals(p3));

print("\nTest 3: Null comparison");
print("p1.equals(null): " + p1.equals(null));

print("\nTest 4: Same reference");
Point p4 = p1;
print("p1.equals(p4): " + p1.equals(p4));
print("p1 == p4: " + (p1 == p4));
