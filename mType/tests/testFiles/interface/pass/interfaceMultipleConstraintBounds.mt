// Test multiple constraint bounds (T extends I1 & I2)
// @Script

interface Drawable {
    function draw(): void;
}

interface Comparable<T> {
    function compareTo(T other): int;
}

// Class that implements both interfaces
class Shape implements Drawable, Comparable<Shape> {
    private int size;

    public constructor(int size) {
        this.size = size;
    }

    public function draw(): void {
        print("Drawing shape of size: " + this.size);
    }

    public function compareTo(Shape other): int {
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
function drawAndCompare(Shape shape, Shape other): void {
    shape.draw();
    int result = shape.compareTo(other);
    if (result > 0) {
        print("First shape is larger");
    } else if (result < 0) {
        print("Second shape is larger");
    } else {
        print("Shapes are equal");
    }
}

Shape s1 = new Shape(10);
Shape s2 = new Shape(20);

drawAndCompare(s1, s2);
