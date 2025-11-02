// Test: Complex constructor overloading with multiple signatures
// Expected: Pass - demonstrates constructor overloading resolution

class Rectangle {
    private int width;
    private int height;
    private string color;
    private bool filled;

    // Constructor 1: All parameters
    public constructor(int width, int height, string color, bool filled) {
        this.width = width;
        this.height = height;
        this.color = color;
        this.filled = filled;
        print("Constructor 1: Full specification");
    }

    // Constructor 2: Size and color only
    public constructor(int width, int height, string color) : this(width, height, color, false) {
        print("Constructor 2: Size and color");
    }

    // Constructor 3: Size only
    public constructor(int width, int height) : this(width, height, "white", false) {
        print("Constructor 3: Size only");
    }

    // Constructor 4: Square with size
    public constructor(int size) : this(size, size, "white", false) {
        print("Constructor 4: Square");
    }

    // Constructor 5: Default
    public constructor() : this(10, 10, "white", false) {
        print("Constructor 5: Default");
    }

    public void display() {
        string fillStatus = this.filled ? "filled" : "not filled";
        print("Rectangle: " + this.width + "x" + this.height + ", " + this.color + ", " + fillStatus);
    }
}

// Test all constructor overloads
print("Test 1 - Full constructor:");
Rectangle r1 = new Rectangle(20, 30, "red", true);
r1.display();

print("\nTest 2 - Size and color:");
Rectangle r2 = new Rectangle(15, 25, "blue");
r2.display();

print("\nTest 3 - Size only:");
Rectangle r3 = new Rectangle(12, 18);
r3.display();

print("\nTest 4 - Square:");
Rectangle r4 = new Rectangle(10);
r4.display();

print("\nTest 5 - Default:");
Rectangle r5 = new Rectangle();
r5.display();
