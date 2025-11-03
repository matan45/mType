// Test: Structural typing compatibility
// Expected: Pass - objects with compatible structure can be used interchangeably

// Different classes with same structure (structural compatibility)
class Point2D {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }

    public function distanceSquared(): int {
        return (this.x * this.x) + (this.y * this.y);
    }
}

class Coordinate {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function toString(): string {
        return "[" + this.x + ", " + this.y + "]";
    }

    public function distanceSquared(): int {
        return (this.x * this.x) + (this.y * this.y);
    }
}

// Test 1: Same structure, different class names
print("Test 1: Structural compatibility");
Point2D point = new Point2D(3, 4);
print("Point: " + point.toString());
print("Distance squared: " + point.distanceSquared());

Coordinate coord = new Coordinate(5, 12);
print("Coordinate: " + coord.toString());
print("Distance squared: " + coord.distanceSquared());

// Test 2: Interface-based structural typing
interface Shape {
    function area(): float;
    function perimeter(): float;
}

class Rectangle implements Shape {
    private float width;
    private float height;

    public constructor(float w, float h) {
        this.width = w;
        this.height = h;
    }

    public function area(): float {
        return this.width * this.height;
    }

    public function perimeter(): float {
        return 2.0 * (this.width + this.height);
    }
}

class Circle implements Shape {
    private float radius;

    public constructor(float r) {
        this.radius = r;
    }

    public function area(): float {
        return 3.14159 * this.radius * this.radius;
    }

    public function perimeter(): float {
        return 2.0 * 3.14159 * this.radius;
    }
}

print("\nTest 2: Interface structural typing");
Shape shape1 = new Rectangle(5.0, 10.0);
print("Rectangle area: " + shape1.area());
print("Rectangle perimeter: " + shape1.perimeter());

Shape shape2 = new Circle(7.0);
print("Circle area: " + shape2.area());
print("Circle perimeter: " + shape2.perimeter());

// Test 3: Array of structurally compatible types via interface
print("\nTest 3: Array of shapes");
Shape[] shapes = new Shape[2];
shapes[0] = new Rectangle(3.0, 4.0);
shapes[1] = new Circle(5.0);

int i = 0;
while (i < 2) {
    print("Shape " + i + " area: " + shapes[i].area());
    i = i + 1;
}

// Test 4: Function accepting structural type (interface)
function calculateTotalArea(Shape[] shapeArray, int count): float {
    float total = 0.0;
    int idx = 0;
    while (idx < count) {
        total = total + shapeArray[idx].area();
        idx = idx + 1;
    }
    return total;
}

print("\nTest 4: Function with structural types");
float totalArea = calculateTotalArea(shapes, 2);
print("Total area: " + totalArea);

print("\nStructural typing tests completed");
