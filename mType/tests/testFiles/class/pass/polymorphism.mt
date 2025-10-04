// Test: Polymorphism - Parent variable holds child instance
// Expected: Pass - demonstrates polymorphic assignment and method dispatch

class Shape {
    public string color;

    public constructor(string color) {
        this.color = color;
    }

    public function draw(): void {
        print("Drawing a shape");
    }

    public function getArea(): float {
        return 0.0;
    }
}

class Circle extends Shape {
    public float radius;

    public constructor(string color, float radius) : super(color) {
        this.radius = radius;
    }

    public function draw(): void {
        print("Drawing a circle with color: " + this.color);
    }

    public function getArea(): float {
        return 3.14159 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    public float width;
    public float height;

    public constructor(string color, float width, float height): super(color) {
        this.width = width;
        this.height = height;
    }

    public function draw(): void {
        print("Drawing a rectangle with color: " + this.color);
    }

    public function getArea(): float {
        return this.width * this.height;
    }
}

// Test polymorphism
Shape shape1 = new Circle("Red", 5.0);
Shape shape2 = new Rectangle("Blue", 4.0, 6.0);

shape1.draw();  // Should call Circle's draw()
shape2.draw();  // Should call Rectangle's draw()

print("Circle area: " + shape1.getArea());
print("Rectangle area: " + shape2.getArea());
