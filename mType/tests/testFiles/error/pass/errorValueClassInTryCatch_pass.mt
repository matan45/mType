// Test: a value class is constructed inside try, inside catch, and
// outside the try; the value held outside survives the throw and the
// in-catch construction is unaffected by the unwind.
import * from "../../lib/exceptions/Exception.mt";

value class Point {
    private int x;
    private int y;
    public constructor(int x, int y) { this.x = x; this.y = y; }
    public function getX(): int { return this.x; }
    public function getY(): int { return this.y; }
    public function toString(): string { return "(" + this.x + ", " + this.y + ")"; }
}

function main(): void {
    Point p1 = new Point(1, 2);
    print("p1: " + p1.toString());
    try {
        Point p2 = new Point(3, 4);
        print("p2: " + p2.toString());
        throw new Exception("test");
    } catch (Exception e) {
        Point p3 = new Point(5, 6);
        print("p3: " + p3.toString() + " caught: " + e.getMessage());
    }
    print("p1 still: " + p1.toString());
}
main();
