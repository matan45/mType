// Test: Value class containing another value class
// Expected: Pass - nested value types maintain copy semantics

value class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
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

value class Rect {
    private Point topLeft;
    private Point bottomRight;

    public constructor(Point tl, Point br) {
        this.topLeft = tl;
        this.bottomRight = br;
    }

    public function getTopLeft(): Point {
        return this.topLeft;
    }

    public function getBottomRight(): Point {
        return this.bottomRight;
    }

    public function width(): int {
        return this.bottomRight.getX() - this.topLeft.getX();
    }

    public function height(): int {
        return this.bottomRight.getY() - this.topLeft.getY();
    }

    public function toString(): string {
        return "Rect[" + this.topLeft.toString() + " -> " + this.bottomRight.toString() + "]";
    }
}

Point p1 = new Point(0, 0);
Point p2 = new Point(10, 5);
Rect r1 = new Rect(p1, p2);

print("r1: " + r1.toString());
print("width: " + r1.width());
print("height: " + r1.height());

// Copy the rect
Rect r2 = r1;
print("r2: " + r2.toString());

// r1 and r2 should be structurally equal
print("r1 == r2: " + (r1 == r2));
