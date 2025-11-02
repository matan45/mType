// Test multiple constraint bounds (T extends I1 & I2)
// @Script

interface Drawable {
    func draw(): void;
}

interface Comparable<T> {
    func compareTo(other: T): Int;
}

// Class that implements both interfaces
class Shape implements Drawable, Comparable<Shape> {
    var size: Int;

    func init(size: Int) {
        this.size = size;
    }

    func draw(): void {
        print("Drawing shape of size: " + this.size.toString());
    }

    func compareTo(other: Shape): Int {
        if (this.size < other.size) {
            return -1;
        }
        if (this.size > other.size) {
            return 1;
        }
        return 0;
    }
}

// Function requiring both interfaces
func drawAndCompare(shape: Shape, other: Shape): void {
    shape.draw();
    var result = shape.compareTo(other);
    if (result > 0) {
        print("First shape is larger");
    } else if (result < 0) {
        print("Second shape is larger");
    } else {
        print("Shapes are equal");
    }
}

var s1 = new Shape(10);
var s2 = new Shape(20);

drawAndCompare(s1, s2);
