// Test: Cast class to interface
interface Drawable {
    function draw(): void;
}

class Circle implements Drawable {
    public function draw(): void {
        print("Drawing circle");
    }
}

Circle circle = new Circle();
Drawable drawable = (Drawable)circle;
drawable.draw();

// Expected output:
// Drawing circle
