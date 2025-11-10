// Test: Casting many objects in a loop (performance & memory pressure test)
// Tests that the VM can handle high-volume casting operations efficiently

interface IShape {
    public function getArea(): float;
    public function getType(): string;
}

class Shape implements IShape {
    public string type;

    public constructor(string t) {
        this.type = t;
    }

    public function getArea(): float {
        return 0.0;
    }

    public function getType(): string {
        return this.type;
    }
}

class Rectangle extends Shape {
    public float width;
    public float height;

    public constructor(float w, float h):super("Rectangle") {
        this.width = w;
        this.height = h;
    }

    public function getArea(): float {
        return this.width * this.height;
    }
}

class Circle extends Shape {
    public float radius;

    public constructor(float r):super("Circle") {
        this.radius = r;
    }

    public function getArea(): float {
        return 3.14159 * this.radius * this.radius;
    }
}

class Triangle extends Shape {
    public float base;
    public float height;

    public constructor(float b, float h):super("Triangle") {
        this.base = b;
        this.height = h;
    }

    public function getArea(): float {
        return 0.5 * this.base * this.height;
    }
}

print("Starting high-volume casting test...");

int totalShapes = 0;
float totalArea = 0.0;
int rectangleCount = 0;
int circleCount = 0;
int triangleCount = 0;

// Create and cast many objects in a loop
int iterations = 100;
int i = 0;

while (i < iterations) {
    // Create different shapes based on iteration
    int shapeType = i % 3;

    if (shapeType == 0) {
        // Create rectangle
        Rectangle rect = new Rectangle(10.0 + (float)i, 5.0 + (float)i);

        // Cast to Shape
        Shape shape = (Shape)rect;
        totalArea = totalArea + shape.getArea();

        // Cast back to Rectangle
        Rectangle rectAgain = (Rectangle)shape;
        if (rectAgain != null) {
            rectangleCount = rectangleCount + 1;
        }
    } else {
        if (shapeType == 1) {
            // Create circle
            Circle circ = new Circle(5.0 + (float)i);

            // Cast to Shape
            Shape shape = (Shape)circ;
            totalArea = totalArea + shape.getArea();

            // Cast back to Circle
            Circle circAgain = (Circle)shape;
            if (circAgain != null) {
                circleCount = circleCount + 1;
            }
        } else {
            // Create triangle
            Triangle tri = new Triangle(8.0 + (float)i, 6.0 + (float)i);

            // Cast to Shape
            Shape shape = (Shape)tri;
            totalArea = totalArea + shape.getArea();

            // Cast back to Triangle
            Triangle triAgain = (Triangle)shape;
            if (triAgain != null) {
                triangleCount = triangleCount + 1;
            }
        }
    }

    totalShapes = totalShapes + 1;
    i = i + 1;
}

print("Completed " + (string)iterations + " iterations");
print("Total shapes created: " + (string)totalShapes);
print("Rectangles: " + (string)rectangleCount);
print("Circles: " + (string)circleCount);
print("Triangles: " + (string)triangleCount);

// Verify total area is positive (actual value may vary due to float precision)
if (totalArea > 0.0) {
    print("Total area calculated: positive");
} else {
    print("Total area calculated: non-positive");
}

// Verify counts match
int expectedTotal = rectangleCount + circleCount + triangleCount;
if (expectedTotal == totalShapes) {
    print("Shape count verification: passed");
} else {
    print("Shape count verification: failed");
}

print("Test completed successfully");

// Expected output:
// Starting high-volume casting test...
// Completed 100 iterations
// Total shapes created: 100
// Rectangles: 34
// Circles: 33
// Triangles: 33
// Total area calculated: positive
// Shape count verification: passed
// Test completed successfully
