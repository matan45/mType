// Test returning null from method expecting interface
// @Script

interface Shape {
    function getArea(): int;
}

class Rectangle implements Shape {
    private int width;
    private int height;

    public constructor(int width, int height) {
        this.width = width;
        this.height = height;
    }

    public function getArea(): int {
        return this.width * this.height;
    }
}

class ShapeFactory {
    public constructor() {
    }

    public function createShape(bool valid): Shape? {
        if (valid) {
            return new Rectangle(10, 20);
        }
        return null;  // Returning null is allowed
    }
}

ShapeFactory factory = new ShapeFactory();

Shape shape1 = factory.createShape(true);
if (shape1 != null) {
    print(shape1.getArea());  // Should print 200
}

Shape shape2 = factory.createShape(false);
if (shape2 == null) {
    print("Shape is null");
}
