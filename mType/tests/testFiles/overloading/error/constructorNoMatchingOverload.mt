// Constructor overload resolution: no constructor accepts (int, int, int).
// Existing no-match coverage is on free functions; this exercises the same
// path through 'new'/constructor selection.
class Point {
    int x;
    int y;

    constructor(int a) {
        this.x = a;
        this.y = 0;
    }

    constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

@Script
function main(): void {
    // ERROR: no matching overload for call to Point constructor with (int, int, int)
    Point p = new Point(1, 2, 3);
    print(p.x);
}
