class Point {
    public int x;
    public constructor(int a) {
        this.x = a;
    }
}

int n = 5;
int sum = 0;
if (n > 0) {
    Point p = new Point(42);
    sum = p.x;
}
print(sum);
