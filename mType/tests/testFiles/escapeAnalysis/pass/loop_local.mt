class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

int total = 0;
for (int i = 0; i < 100; i = i + 1) {
    Point p = new Point(i);
    total = total + p.x;
}
print(total);
