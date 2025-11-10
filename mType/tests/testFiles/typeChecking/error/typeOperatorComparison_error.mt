// ERROR: Comparing unrelated types without proper conversion

class Point {
    int x;
    int y;

    constructor(int xVal, int yVal) {
        x = xVal;
        y = yVal;
    }
}

class Color {
    string name;

    constructor(string n) {
        name = n;
    }
}


function main(): void {
    Point p = new Point(10, 20);
    Color c = new Color("Red");

    // ERROR: Cannot compare unrelated object types
    bool result = p == c;

    print("This should not execute");
}

main();
