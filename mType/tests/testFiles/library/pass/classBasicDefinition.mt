class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function toString(): string {
        return "Point(" + this.x + ", " + this.y + ")";
    }
}

Point p = new Point(3, 4);
print(p.toString());
print(p.x);
print(p.y);
