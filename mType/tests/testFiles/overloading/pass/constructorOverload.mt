// Constructor overloading
class Point {
    int x;
    int y;

    // Default constructor
    constructor() {
        this.x = 0;
        this.y = 0;
    }

    // Constructor with one parameter
    constructor(int val) {
        this.x = val;
        this.y = val;
    }

    // Constructor with two parameters
    constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function toString(): string {
        return "Point(" + this.x + ", " + this.y + ")";
    }
}


function main(): void {
    Point p1 = new Point();          // Should call Point()
    print(p1.toString());

    Point p2 = new Point(5);         // Should call Point(int)
    print(p2.toString());

    Point p3 = new Point(10, 20);    // Should call Point(int, int)
    print(p3.toString());
}

main();
