// Test: Basic value class definition and usage
// Expected: Pass - value classes can be defined, instantiated, and used

value class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public constructor() {
        this.x = 0;
        this.y = 0;
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

// Basic construction
Point p1 = new Point(3, 4);
print("p1: " + p1.toString());
print("x: " + p1.getX());
print("y: " + p1.getY());

// Default constructor
Point p2 = new Point();
print("p2: " + p2.toString());

// Null value class reference
Point? p3 = null;
print("p3 is null: " + (p3 == null));
