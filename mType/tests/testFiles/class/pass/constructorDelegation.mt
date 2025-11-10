// Test: Constructor delegation using this() to call another constructor
// Expected: Pass - demonstrates constructor delegation pattern

class Point {
    public int x;
    public int y;
    public string label;

    // Main constructor
    public constructor(int x, int y, string label) {
        this.x = x;
        this.y = y;
        this.label = label;
        print("Full constructor: (" + x + ", " + y + ") - " + label);
    }

    // Delegating constructor - calls main constructor with default label
    public constructor(int x, int y) {
	 this(x, y, "Point");
        print("Delegated constructor with default label");
    }

    // Delegating constructor - calls other constructor with origin
    public constructor() {
	this(0, 0);
        print("Default constructor - origin point");
    }
}

// Test all constructors
print("Creating point with full constructor:");
Point p1 = new Point(5, 10, "Custom");
print("Result: (" + p1.x + ", " + p1.y + ") - " + p1.label);

print("\nCreating point with two-arg constructor:");
Point p2 = new Point(3, 7);
print("Result: (" + p2.x + ", " + p2.y + ") - " + p2.label);

print("\nCreating point with default constructor:");
Point p3 = new Point();
print("Result: (" + p3.x + ", " + p3.y + ") - " + p3.label);
