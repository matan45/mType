// Test: Classes & Inheritance in standalone exe

abstract class Shape {
    protected string color;

    public constructor(string c) {
        this.color = c;
    }

    abstract function area(): float;

    public function describe(): string {
        return this.color + " shape with area " + this.area();
    }
}

class Circle extends Shape {
    private float radius;

    public constructor(string c, float r) : super(c) {
        this.radius = r;
    }

    public function area(): float {
        return 3.14 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    private float width;
    private float height;

    public constructor(string c, float w, float h) : super(c) {
        this.width = w;
        this.height = h;
    }

    public function area(): float {
        return this.width * this.height;
    }
}

class Square extends Rectangle {
    public constructor(string c, float side) : super(c, side, side) {
    }
}

class Counter {
    private static int count = 0;

    public static function increment(): void {
        Counter.count = Counter.count + 1;
    }

    public static function getCount(): int {
        return Counter.count;
    }
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Abstract class + inheritance
        Circle c = new Circle("red", 5.0);
        print("Circle: " + c.describe());

        Rectangle r = new Rectangle("blue", 4.0, 6.0);
        print("Rectangle: " + r.describe());

        // Multi-level inheritance
        Square s = new Square("green", 3.0);
        print("Square: " + s.describe());

        // Polymorphism
        Shape shape = c;
        print("Polymorphic area: " + shape.area());

        // Static fields and methods
        Counter::increment();
        Counter::increment();
        Counter::increment();
        print("Counter: " + Counter::getCount());

        print("Classes test passed");
    }
}
