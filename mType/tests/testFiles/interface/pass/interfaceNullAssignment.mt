// Test null assignment to interface variable
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

// Null assignment should be allowed
var drawable: Drawable = null;

if (drawable == null) {
    print("Drawable is null");
}

// Assign non-null value
drawable = new Circle(5);

if (drawable != null) {
    print("Drawable is not null");
    drawable.draw();
}

// Reassign to null
drawable = null;

if (drawable == null) {
    print("Drawable is null again");
}
