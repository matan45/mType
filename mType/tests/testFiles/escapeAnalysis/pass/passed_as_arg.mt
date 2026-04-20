class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

function readX(Point p): int {
    return p.x;
}

Point q = new Point(55);
print(readX(q));
