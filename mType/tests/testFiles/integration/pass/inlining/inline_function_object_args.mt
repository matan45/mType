// MYT-210: object-typed parameter exercises the BOXED dispatch in
// emitInlineLocalCopy (raw memcpy + tag-reset donate path) and the
// matching emitInlineLocalDestroy release. The callee reads a field on
// the donated boxed local — so the donation + remap must round-trip
// correctly across the inline frame.

class Point {
    public int x;
    public int y;
    constructor(int x_, int y_) {
        this.x = x_;
        this.y = y_;
    }
}

function magnitudeSq(Point p): int {
    return p.x * p.x + p.y * p.y;
}

Point a = new Point(3, 4);
Point b = new Point(5, 12);
print(magnitudeSq(a));   // 25
print(magnitudeSq(b));   // 169
