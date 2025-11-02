// Test invalid interface to class cast
// @Throw

interface Shape {
    func getArea(): Int;
}

class Rectangle {
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

class Circle implements Shape {
    var radius: Int;

    func init(radius: Int) {
        this.radius = radius;
    }

    func getArea(): Int {
        return 3 * this.radius * this.radius;
    }
}

var shape: Shape = new Circle(5);

// Error: Cannot cast interface to unrelated class
var rect: Rectangle = shape as Rectangle;
print(rect.width);
