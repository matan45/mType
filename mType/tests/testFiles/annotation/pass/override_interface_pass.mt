// Test: @Override annotation with interface method
// Expected: Pass - method correctly implements interface method

interface Drawable {
    void draw();
    void resize(Int width, Int height);
}

class Circle implements Drawable {
    @Override
    void draw() {
        print("Drawing circle");
    }

    @Override
    void resize(Int width, Int height) {
        print("Resizing circle");
    }
}

// Test execution
Circle circle = new Circle();
circle.draw();
circle.resize(100, 100);
