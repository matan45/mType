// Test promise covariance (Promise<Derived> to Promise<Base>)

import { Int } from "../../lib/primitives/Int.mt";
import { String } from "../../lib/primitives/String.mt";

print("=== Async Promise Covariance Test ===");

class Shape {
    protected string name;

    public constructor(string n) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }

    public function async describe(): Promise<String> {
        return new String("Shape: " + this.name);
    }
}

class Circle extends Shape {
    int radius;

    public constructor(string n, int r):super(n) {
        this.radius = r;
    }

    public function getRadius(): int {
        return this.radius;
    }

    public function async describe(): Promise<String> {
        return new String("Circle: " + this.name + " (radius: " + this.radius + ")");
    }
}

function async createCircle(): Promise<Circle> {
    print("Creating circle");
    Circle c = new Circle("MyCircle", 10);
    return c;
}

function async processShape(Shape shape): Promise<String> {
    print("Processing shape: " + shape.getName());
    string desc = await shape.describe();
    print("Description: " + desc);
    return new String(desc);
}

function async main(): Promise<String> {
    // Promise<Circle> can be used where Promise<Shape> is expected
    Circle circle = await createCircle();

    // Upcast to Shape
    Shape shape = circle;
    string result = (await processShape(shape)).getValue();

    return new String(result);
}

main();
