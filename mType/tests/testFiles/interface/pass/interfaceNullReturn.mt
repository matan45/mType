// Test returning null from method expecting interface
// @Script

interface Shape {
    func getArea(): Int;
}

class Rectangle implements Shape {
    var width: Int;
    var height: Int;

    func init(width: Int, height: Int) {
        this.width = width;
        this.height = height;
    }

    func getArea(): Int {
        return this.width * this.height;
    }
}

class ShapeFactory {
    func createShape(valid: Bool): Shape {
        if (valid) {
            return new Rectangle(10, 20);
        }
        return null;  // Returning null is allowed
    }
}

var factory = new ShapeFactory();

var shape1 = factory.createShape(true);
if (shape1 != null) {
    print(shape1.getArea());  // Should print 200
}

var shape2 = factory.createShape(false);
if (shape2 == null) {
    print("Shape is null");
}
