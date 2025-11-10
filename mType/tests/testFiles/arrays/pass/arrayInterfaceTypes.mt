// Test arrays of interface types
print("Testing arrays of interface types");

interface Drawable {
    function draw(): string;
}

class Circle implements Drawable {
    int radius;

    constructor(int r) {
        radius = r;
    }

    public function draw(): string {
        return "Circle with radius " + radius;
    }
}

class Rectangle implements Drawable {
    int width;
    int height;

    constructor(int w, int h) {
        width = w;
        height = h;
    }

    public function draw(): string {
        return "Rectangle " + width + "x" + height;
    }
}

Drawable[] shapes = new Drawable[3];
shapes[0] = new Circle(5);
shapes[1] = new Rectangle(10, 20);
shapes[2] = new Circle(3);

print("Drawing shapes:");
for (int i = 0; i < shapes.length; i++) {
    print("  " + shapes[i].draw());
}

print("Interface types array test completed");
