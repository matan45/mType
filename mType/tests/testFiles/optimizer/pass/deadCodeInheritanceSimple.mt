// Simple test for Dead Code Elimination with Inheritance and Interfaces
// Tests that extends and implements are preserved while dead code is removed

interface Drawable {
    function draw(): void;
}

class Shape {
    protected string color;

    public constructor(string color) {
        this.color = color;
    }

    public function getColor(): string {
        return this.color;
        print("Dead after return in base class");  // Should be removed
    }
}

class Rectangle extends Shape implements Drawable {
    private int width;
    private int height;

    public constructor(string color, int w, int h) : super(color) {
        this.width = w;
        this.height = h;
    }

    public function draw(): void {
        print("Drawing rectangle: " + this.color);
        return;
        print("Dead after return in draw");  // Should be removed
    }

    public function getArea(): int {
        int area = this.width * this.height;
        return area;
        print("Dead after return in getArea");  // Should be removed
    }
}

// Test execution
Rectangle rect = new Rectangle("blue", 10, 20);
print("Color: " + rect.getColor());
rect.draw();
print("Area: " + rect.getArea());

print("Test Complete!");
