// Test: Mix of final and non-final methods - Polymorphism with final methods
// Expected: Pass - final methods prevent overriding, non-final methods allow it

class Shape {
    public final function getCategory(): string {
        return "Shape";
    }

    public function getArea(): int {
        return 0;
    }

    public function describe(): string {
        return "Generic shape";
    }
}

class Rectangle extends Shape {
    public int width;
    public int height;

    public constructor(int w, int h) {
        this.width = w;
        this.height = h;
    }

    // Cannot override getCategory() - it's final
    // Can override getArea() and describe() - they're not final

    public function getArea(): int {
        return this.width * this.height;
    }

    public function describe(): string {
        return "Rectangle " + this.width + "x" + this.height;
    }
}

class Circle extends Shape {
    public int radius;

    public constructor(int r) {
        this.radius = r;
    }

    // Cannot override getCategory() - it's final
    // Can override getArea() and describe() - they're not final

    public function getArea(): int {
        // Approximation: pi * r^2 (using 3 for pi)
        return 3 * this.radius * this.radius;
    }

    public function describe(): string {
        return "Circle with radius " + this.radius;
    }
}

// Test polymorphism with final and non-final methods
Shape s = new Shape();
print(s.getCategory());  // Should print: Shape
print(s.getArea());  // Should print: 0
print(s.describe());  // Should print: Generic shape

Rectangle r = new Rectangle(5, 4);
print(r.getCategory());  // Should print: Shape (final, inherited)
print(r.getArea());  // Should print: 20 (overridden)
print(r.describe());  // Should print: Rectangle 5x4 (overridden)

Circle c = new Circle(3);
print(c.getCategory());  // Should print: Shape (final, inherited)
print(c.getArea());  // Should print: 27 (overridden)
print(c.describe());  // Should print: Circle with radius 3 (overridden)
