// MYT-274: synthesized hashCode is reflexive (same object -> same hash) and
// content-consistent (same field values -> same hash). Bit values are
// implementation-defined, so we only assert structural properties.

class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

Point a = new Point(3, 7);
Point b = new Point(3, 7);
Point c = new Point(4, 7);

print("reflexive: " + (a.hashCode() == a.hashCode()));
print("equal contents: " + (a.hashCode() == b.hashCode()));
print("differing contents likely differ: " + (a.hashCode() != c.hashCode()));
