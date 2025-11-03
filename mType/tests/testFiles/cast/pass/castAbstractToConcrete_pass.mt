// Test abstract to concrete class casting

abstract class Shape {
    public abstract function getArea(): float;
}

class Circle extends Shape {
    private float radius;

    constructor(float r) {
        this.radius = r;
    }

    public function getArea(): float {
        return 3.14 * this.radius * this.radius;
    }

    public function getRadius(): float {
        return this.radius;
    }
}

class Rectangle extends Shape {
    private float width;
    private float height;

    constructor(float w, float h): super(1) {
        this.width = w;
        this.height = h;
    }

    public function getArea(): float {
        return this.width * this.height;
    }
}

function processShape(Shape shape): float {
    // Attempt to downcast to Circle
    Circle circle = (Circle)shape;
    if (circle != null) {
        return circle.getRadius();
    }
    return 0.0;
}


function main(): void {
    Circle circle = new Circle(5.0);
    Shape shape = circle;  // Upcast

    print(shape.getArea());  // Should work on abstract reference

    Circle concrete = (Circle)shape;  // Downcast back
    print(concrete.getRadius());

    float result = processShape(shape);
    print(result);
}
main();
