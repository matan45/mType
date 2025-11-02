@Script
// Test abstract to concrete class casting
abstract class Shape {
    abstract fn getArea(): Float;
}

class Circle : Shape {
    let radius: Float;

    fn init(r: Float) {
        this.radius = r;
    }

    fn getArea(): Float {
        return 3.14 * this.radius * this.radius;
    }

    fn getRadius(): Float {
        return this.radius;
    }
}

class Rectangle : Shape {
    let width: Float;
    let height: Float;

    fn init(w: Float, h: Float) {
        this.width = w;
        this.height = h;
    }

    fn getArea(): Float {
        return this.width * this.height;
    }
}

fn processShape(shape: Shape): Float {
    // Attempt to downcast to Circle
    let circle: Circle = shape as Circle;
    if (circle != null) {
        return circle.getRadius();
    }
    return 0.0;
}

fn main() {
    let circle: Circle = new Circle(5.0);
    let shape: Shape = circle as Shape;  // Upcast

    print(shape.getArea());  // Should work on abstract reference

    let concrete: Circle = shape as Circle;  // Downcast back
    print(concrete.getRadius());

    let result: Float = processShape(shape);
    print(result);
}
