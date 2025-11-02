// Test promise covariance (Promise<Derived> to Promise<Base>)

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Promise Covariance Test ===");

class Shape {
    string name;

    public constructor(string n) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }

    public function async describe(): Promise<string> {
        return "Shape: " + this.name;
    }
}

class Circle extends Shape {
    int radius;

    public constructor(string n, int r) {
        super(n);
        this.radius = r;
    }

    public function getRadius(): int {
        return this.radius;
    }

    public function async describe(): Promise<string> {
        return "Circle: " + this.name + " (radius: " + this.radius + ")";
    }
}

function async createCircle(): Promise<Circle> {
    print("Creating circle");
    Circle c = new Circle("MyCircle", 10);
    return c;
}

function async processShape(Shape shape): Promise<string> {
    print("Processing shape: " + shape.getName());
    string desc = await shape.describe();
    print("Description: " + desc);
    return desc;
}

function async main(): Promise<string> {
    // Promise<Circle> can be used where Promise<Shape> is expected
    Circle circle = await createCircle();

    // Upcast to Shape
    Shape shape = circle;
    string result = await processShape(shape);

    return result;
}

main();
