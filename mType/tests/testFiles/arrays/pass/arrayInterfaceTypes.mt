// Test arrays of interface types
print("Testing arrays of interface types");

interface Drawable {
    string draw();
}

class Circle : Drawable {
    int radius;

    Circle(int r) {
        radius = r;
    }

    string draw() {
        return "Circle with radius " + radius;
    }
}

class Rectangle : Drawable {
    int width;
    int height;

    Rectangle(int w, int h) {
        width = w;
        height = h;
    }

    string draw() {
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
