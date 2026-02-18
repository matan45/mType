// Test: Value class operations under JIT compilation
// Functions called 200+ times to trigger JIT compilation (threshold=100)
// Exercises: NEW_VALUE_OBJECT, OBJECT_TO_VALUE, field access, method calls

value class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function add(Point other): Point {
        return new Point(this.x + other.x, this.y + other.y);
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

// Hot function: value class method call returning string (stays BOXED throughout)
function pointToString(Point p): string {
    return p.toString();
}

// Hot function: value class add returning value object (BOXED in, BOXED out)
function addPoints(Point a, Point b): Point {
    return a.add(b);
}

// Hot function: pure int arithmetic (no value objects in JIT path)
function computeSum(int x, int y): int {
    return x * 3 + y * 2;
}

// === Test 1: Hot loop — value class toString (BOXED path) ===
Point p1 = new Point(5, 10);
string lastStr = "";
for (int i = 0; i < 200; i = i + 1) {
    lastStr = pointToString(p1);
}
print("toString: " + lastStr);

// === Test 2: Hot loop — value class add method (BOXED in/out) ===
Point accumulated = new Point(0, 0);
Point step = new Point(1, 2);
for (int i = 0; i < 200; i = i + 1) {
    accumulated = addPoints(accumulated, step);
}
print("accumulated: " + accumulated.toString());

// === Test 3: Hot loop — int arithmetic called with value object fields ===
// Value object field access in interpreter, hot int function in JIT
int total = 0;
for (int i = 0; i < 200; i = i + 1) {
    Point p = new Point(i, i * 2);
    total = total + computeSum(p.x, p.y);
}
// total = sum of (i*3 + i*2*2) = sum of (7*i) for i=0..199 = 7 * 199*200/2 = 139300
print("total: " + total);

// === Test 4: Value class construction in hot loop ===
string lastPoint = "";
for (int i = 0; i < 200; i = i + 1) {
    Point p = new Point(i, i + 1);
    lastPoint = pointToString(p);
}
print("last point: " + lastPoint);

// === Test 5: Value class conditional path ===
function pickPoint(bool first, Point a, Point b): Point {
    if (first) {
        return a;
    }
    return b;
}

Point pA = new Point(10, 20);
Point pB = new Point(30, 40);
Point picked = new Point(0, 0);
for (int i = 0; i < 200; i = i + 1) {
    picked = pickPoint(i % 2 == 0, pA, pB);
}
// Last iteration i=199 is odd, so picked = pB
print("last picked: " + picked.toString());

print("Value class JIT performance tests passed!");
