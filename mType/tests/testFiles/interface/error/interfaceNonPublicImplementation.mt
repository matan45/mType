// Test implementing interface with non-public method (should error)
// @Throw

interface Drawable {
    func draw(): void;
}

class Circle implements Drawable {
    var radius: Int;

    func init(radius: Int) {
        this.radius = radius;
    }

    // Error: Interface methods must be public, but this is not explicitly public
    // In most languages, interface implementation methods must be public
    private func draw(): void {
        print("Drawing circle");
    }
}

var circle = new Circle(5);
circle.draw();  // Should fail - method is private
