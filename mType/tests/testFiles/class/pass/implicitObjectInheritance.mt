// Test: Implicit Object inheritance
// Expected: Pass - all classes automatically inherit toString, equals, hashCode from Object

// Plain class with no extends, no implements — should have Object methods via implicit inheritance
class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

// Class that overrides Object methods
class NamedPoint {
    public int x;
    public int y;
    public string label;

    public constructor(int x, int y, string label) {
        this.x = x;
        this.y = y;
        this.label = label;
    }

    public function toString(): string {
        return this.label + "(" + this.x + ", " + this.y + ")";
    }

    public function hashCode(): int {
        return this.x * 31 + this.y;
    }
}

// Test 1: Default Object methods on a plain class
Point p1 = new Point(3, 4);
Point p2 = new Point(3, 4);
Point p3 = new Point(5, 6);

// Default equals uses content-based comparison
print("p1 equals p2: " + p1.equals(p2));
print("p1 equals p3: " + p1.equals(p3));

// Default hashCode returns an int
int h1 = p1.hashCode();
int h2 = p2.hashCode();
print("p1 hashCode == p2 hashCode: " + (h1 == h2));

// Default toString returns ClassName@hashCode format
string str = p1.toString();
// Verify it's not empty (contains ClassName@hash)
print("p1 toString not empty: " + (str != ""));

// Test 2: Overridden Object methods
NamedPoint np = new NamedPoint(1, 2, "Origin");
print("np toString: " + np.toString());
print("np hashCode: " + np.hashCode());

// Test 3: Class with explicit extends also has Object at root
class Shape {
    public string type;

    public constructor(string type) {
        this.type = type;
    }

    public function toString(): string {
        return "Shape:" + this.type;
    }
}

class Circle extends Shape {
    public int radius;

    public constructor(int radius) : super("circle") {
        this.radius = radius;
    }
}

Circle c = new Circle(5);
// toString is inherited from Shape (which overrides Object)
print("circle toString: " + c.toString());
// hashCode comes from Object (through Shape)
int ch = c.hashCode();
print("circle has hashCode: " + (ch != 0));
