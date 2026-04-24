class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

Point p = new Point(7);
Point q = p;
q.x = 11;
print(p.x);
print(q.x);
