// Test null assignment to interface variable
// @Script

interface Drawable {
    function draw(): void;
}

class Circle implements Drawable {
    private int radius;

    public constructor(int radius) {
        this.radius = radius;
    }

    public function draw(): void {
        print("Drawing circle with radius: " + this.radius);
    }
}

// Null assignment should be allowed
Drawable drawable = null;

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
