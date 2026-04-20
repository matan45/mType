class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

function makeAliased(int v): Point {
    Point p = new Point(v);
    Point q = p;
    return q;
}

Point r = makeAliased(77);
print(r.x);
