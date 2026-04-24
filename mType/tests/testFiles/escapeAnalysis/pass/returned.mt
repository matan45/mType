class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

function makePoint(int v): Point {
    Point p = new Point(v);
    return p;
}

Point r = makePoint(99);
print(r.x);
