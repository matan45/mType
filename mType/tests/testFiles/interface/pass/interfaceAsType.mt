// Interface used as type test
interface Drawable {
    function draw(): void;
    function getArea(): float;
}

interface Shape {
    function getPerimeter(): float;
}

class Circle implements Drawable {
    private float radius;

    public constructor(float r) {
        this.radius = r;
    }

    public function draw(): void {
        print("Drawing circle");
    }

    public function getArea(): float {
        return 3.14159 * this.radius * this.radius;
    }
}

class ShapeProcessor {
    // Interface as data member
    private Drawable currentShape;

    public constructor() {
        this.currentShape = null;
    }

    // Interface as parameter type
    public function processShape(Drawable shape): void {
        shape.draw();
        print("Processing shape with area: " + shape.getArea());
    }

    // Interface as return type
    public function getCurrentShape(): Drawable {
        return this.currentShape;
    }

    // Interface as local variable
    public function demonstrateLocalVariable(): void {
        Drawable localShape = new Circle(2.5);
        localShape.draw();
        this.currentShape = localShape;
    }

    // Static method with interface parameter
    public static function compareShapes(Drawable shape1, Drawable shape2): bool {
        return shape1.getArea() > shape2.getArea();
    }
}

// Test interface as types
ShapeProcessor processor = new ShapeProcessor();
processor.demonstrateLocalVariable();

Circle circle = new Circle(5.0);
processor.processShape(circle);

Drawable retrieved = processor.getCurrentShape();
retrieved.draw();

bool isLarger = ShapeProcessor::compareShapes(circle, retrieved);
print("Circle is larger: " + isLarger);

print("Interface as type successful");
