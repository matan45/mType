// Test: Basic abstract class with abstract method
// This should compile and run without errors

abstract class Shape {
    abstract function getArea(): float;

    public function describe(): void {
        print("I am a shape");
    }
}

class Circle extends Shape {
    private float radius;

    constructor(float r) {
        radius = r;
    }

    public function getArea(): float {
        return 3.14 * radius * radius;
    }
}

class Rectangle extends Shape {
    private float width;
    private float height;

    constructor(float w, float h) {
        width = w;
        height = h;
    }

    public function getArea(): float {
        return width * height;
    }
}

// Test the implementation
Circle c = new Circle(5.0);
print("Circle area: " + c.getArea());
c.describe();

Rectangle r = new Rectangle(4.0, 6.0);
print("Rectangle area: " + r.getArea());
r.describe();
