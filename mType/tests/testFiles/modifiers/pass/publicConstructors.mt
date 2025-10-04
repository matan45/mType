// Test: Public constructors accessible externally
class Point {
    public int x;
    public int y;

    public constructor() {
        x = 0;
        y = 0;
    }

    public constructor(int px, int py) {
        x = px;
        y = py;
    }
}

// Public constructors can be called from anywhere
Point p1 = new Point();
print(p1.x);  // Expected: 0
print(p1.y);  // Expected: 0

Point p2 = new Point(10, 20);
print(p2.x);  // Expected: 10
print(p2.y);  // Expected: 20
