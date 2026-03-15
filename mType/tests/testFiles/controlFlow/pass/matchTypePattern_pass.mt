// Test: match statement with type patterns and variable binding

abstract class Shape {
    abstract function area(): float;
    abstract function name(): string;
}

class Circle extends Shape {
    private float radius;
    constructor(float r) { this.radius = r; }
    public function area(): float { return 3.14 * this.radius * this.radius; }
    public function name(): string { return "Circle"; }
}

class Rectangle extends Shape {
    private float width;
    private float height;
    constructor(float w, float h) { this.width = w; this.height = h; }
    public function area(): float { return this.width * this.height; }
    public function name(): string { return "Rectangle"; }
}

function describeShape(Shape shape): void {
    match (shape) {
        case Circle c -> print("Circle with area: " + c.area());
        case Rectangle r -> print("Rectangle with area: " + r.area());
        default -> print("Unknown shape");
    }
}

function main(): void {
    Shape circle = new Circle(5.0);
    Shape rect = new Rectangle(3.0, 4.0);

    describeShape(circle);
    describeShape(rect);
}
main();
