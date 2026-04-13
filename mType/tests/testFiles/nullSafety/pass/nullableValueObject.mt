// Test: Nullable types with value classes

value class Point {
    private int x;
    private int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int {
        return this.x;
    }

    public function getY(): int {
        return this.y;
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

// Nullable value class variable
Point? p1 = null;
print("p1 is null: " + (p1 == null));

// Assign value object to nullable
Point p2 = new Point(3, 4);
print("p2: " + p2.toString());

// Non-nullable value class
Point p3 = new Point(1, 2);
print("p3: " + p3.toString());

// Smart cast with value class
if (p2 != null) {
    print("p2 x: " + p2.getX());
    print("p2 y: " + p2.getY());
}

// Function with nullable value class param
function describePoint(Point? pt): string {
    if (pt != null) {
        return pt.toString();
    }
    return "no point";
}

print(describePoint(new Point(10, 20)));
print(describePoint(null));

// Function returning nullable value class
function findPoint(bool exists): Point? {
    if (exists) {
        return new Point(5, 6);
    }
    return null;
}

Point? found = findPoint(true);
if (found != null) {
    print("found: " + found.toString());
}

Point? missing = findPoint(false);
print("missing is null: " + (missing == null));

// Reassign nullable value class
p1 = new Point(7, 8);
if (p1 != null) {
    print("p1 after assign: " + p1.toString());
}
p1 = null;
print("p1 after null: " + (p1 == null));

print("Nullable value object tests passed!");
