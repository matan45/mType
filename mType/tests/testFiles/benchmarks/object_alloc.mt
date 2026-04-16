// TARGET: ~2s on dev machine. Adjust N if first run lands outside 1-5s.
// Exercises allocation + short-lived object lifecycle. Future escape-analysis
// work (epic MYT-118) should show large wins here.

class Point {
    public int x;
    public int y;
    public constructor(int a, int b) {
        this.x = a;
        this.y = b;
    }
}

int N = 2000000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    Point p = new Point(i, i + 1);
    total = total + p.x;
}

print("object_alloc total=" + total);
