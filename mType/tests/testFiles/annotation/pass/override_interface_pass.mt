// Test: @Override annotation with interface method
// Expected: Pass - method correctly implements interface method

interface Drawable {
    function draw(): void;
    function resize(int width, int height): void;
}

class Circle implements Drawable {
    @Override
    public function draw(): void {
        print("Drawing circle");
    }

    @Override
    public function resize(int width, int height): void {
        print("Resizing circle");
    }
}

// Test execution
Circle circle = new Circle();
circle.draw();
circle.resize(100, 100);
