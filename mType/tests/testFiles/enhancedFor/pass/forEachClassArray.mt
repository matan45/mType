// Test enhanced for-loop over an array of a user-defined class
class Point {
    private int x;
    private int y;

    constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int {
        return this.x;
    }

    public function getY(): int {
        return this.y;
    }
}

function main(): void {
    print("Testing enhanced for-loop over Point[]:");

    Point[] points = new Point[3];
    points[0] = new Point(1, 2);
    points[1] = new Point(3, 4);
    points[2] = new Point(5, 6);

    int sumX = 0;
    int sumY = 0;
    for (Point p : points) {
        print(p.getX() + "," + p.getY());
        sumX = sumX + p.getX();
        sumY = sumY + p.getY();
    }
    print("sumX: " + sumX);
    print("sumY: " + sumY);

    print("for-each over Point[] test passed!");
}

main();
