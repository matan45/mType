// Test: Cast class to interface
interface Drawable {
    void draw();
}

class Circle implements Drawable {
    void draw() {
        print("Drawing circle");
    }
}

Circle circle = new Circle();
Drawable drawable = (Drawable)circle;
drawable.draw();

// Expected output:
// Drawing circle
