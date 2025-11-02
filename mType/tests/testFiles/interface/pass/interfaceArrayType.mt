// Test interface array type
// @Script

interface Drawable {
    func draw(): void;
}

class Circle implements Drawable {
    var radius: Int;

    func init(radius: Int) {
        this.radius = radius;
    }

    func draw(): void {
        print("Drawing circle with radius: " + this.radius.toString());
    }
}

class Rectangle implements Drawable {
    var width: Int;
    var height: Int;

    func init(width: Int, height: Int) {
        this.width = width;
        this.height = height;
    }

    func draw(): void {
        print("Drawing rectangle " + this.width.toString() + "x" + this.height.toString());
    }
}

// Array of interface type
var shapes = new Array<Drawable>();
shapes.add(new Circle(5));
shapes.add(new Rectangle(10, 20));
shapes.add(new Circle(15));

// Draw all shapes
var i = 0;
while (i < shapes.size()) {
    var shape = shapes.get(i);
    shape.draw();
    i = i + 1;
}
