// Test: Safe downcasting with runtime type checking
// Expected: Pass - demonstrates proper downcast with instanceof checks

class Shape {
    protected string color;

    public constructor(string color) {
        this.color = color;
    }

    public function draw(): void {
        print("Drawing shape in " + this.color);
    }

    public function getColor(): string {
        return this.color;
    }
}

class Rectangle extends Shape {
    private int width;
    private int height;

    public constructor(string color, int width, int height) : super(color) {
        this.width = width;
        this.height = height;
    }

    public function draw(): void {
        print("Drawing " + this.color + " rectangle: " + this.width + "x" + this.height);
    }

    public function getArea(): int {
        return this.width * this.height;
    }

    public function getWidth(): int {
        return this.width;
    }

    public function getHeight(): int {
        return this.height;
    }
}

class Circle extends Shape {
    private int radius;

    public constructor(string color, int radius) : super(color) {
        this.radius = radius;
    }

    public function draw(): void {
        print("Drawing " + this.color + " circle with radius " + this.radius);
    }

    public function getRadius(): int {
        return this.radius;
    }
}

// Test 1: Safe downcast with instanceof - Rectangle
print("Test 1: Downcast Rectangle");
Shape s1 = new Rectangle("red", 10, 5);
s1.draw();

if (s1 isClassOf Rectangle) {
    Rectangle rect = (Rectangle)s1;
    print("Successfully downcast to Rectangle");
    print("Area: " + rect.getArea());
    print("Width: " + rect.getWidth());
    print("Height: " + rect.getHeight());
}

// Test 2: Safe downcast with instanceof - Circle
print("\nTest 2: Downcast Circle");
Shape s2 = new Circle("blue", 7);
s2.draw();

if (s2 isClassOf Circle) {
    Circle circ = (Circle)s2;
    print("Successfully downcast to Circle");
    print("Radius: " + circ.getRadius());
}

// Test 3: Type checking prevents wrong downcast
print("\nTest 3: Type safety check");
Shape s3 = new Rectangle("green", 8, 4);

if (s3 isClassOf Circle) {
    print("This should not print - s3 is not a Circle");
} else if (s3 isClassOf Rectangle) {
    print("Correctly identified as Rectangle");
    Rectangle r = (Rectangle)s3;
    print("Area: " + r.getArea());
}

// Test 4: Multiple downcasts in array processing
print("\nTest 4: Array with mixed types");
Shape[] shapes = new Shape[4];
shapes[0] = new Rectangle("yellow", 6, 3);
shapes[1] = new Circle("purple", 5);
shapes[2] = new Rectangle("orange", 4, 4);
shapes[3] = new Circle("pink", 3);

int i = 0;
int totalRectArea = 0;
int totalCircleCount = 0;

while (i < 4) {
    if (shapes[i] isClassOf Rectangle) {
        Rectangle r = (Rectangle)shapes[i];
        totalRectArea = totalRectArea + r.getArea();
        print("Found rectangle with area: " + r.getArea());
    } else if (shapes[i] isClassOf Circle) {
        Circle c = (Circle)shapes[i];
        totalCircleCount = totalCircleCount + 1;
        print("Found circle with radius: " + c.getRadius());
    }
    i = i + 1;
}

print("Total rectangle area: " + totalRectArea);
print("Total circles found: " + totalCircleCount);

print("\nAll downcast tests completed");
