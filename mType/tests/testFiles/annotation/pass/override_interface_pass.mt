// Test: @Override annotation with interface method
// Expected: Pass - method correctly implements interface method

interface Drawable {
    function draw(): void;
    function resize(Int width, Int height): void;
}

class Circle implements Drawable {
    @Override
    function draw(): void {
        print("Drawing circle");
    }

    @Override
    function resize(Int width, Int height): void {
        print("Resizing circle");
    }
}

// Test execution
Circle circle = new Circle();
circle.draw();
circle.resize(100, 100);
