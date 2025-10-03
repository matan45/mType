// Test: Casting with method override (polymorphism)
class Shape {
    function draw(): void {
        print("Drawing shape");
    }
}

class Circle extends Shape {
    function draw(): void {
        print("Drawing circle");
    }
}

Circle circle = new Circle();
Shape shape = (Shape)circle;  // Upcast
shape.draw();  // Expected: Drawing circle (polymorphic call)

Shape baseShape = new Shape();
baseShape.draw();  // Expected: Drawing shape

// Expected output:
// Drawing circle
// Drawing shape
