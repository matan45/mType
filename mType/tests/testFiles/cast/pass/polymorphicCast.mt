// Test: Polymorphic behavior with casting
class Shape {
    function area(): void {
        print("Unknown area");
    }
}

class Rectangle extends Shape {
    int width;
    int height;

    constructor(int w, int h) {
        this.width = w;
        this.height = h;
    }

    function area():void {
        print(this.width * this.height);
    }
}

Rectangle rect = new Rectangle(5, 10);
Shape shape = (Shape)rect;
shape.area();  // Expected: 50 (polymorphic call)

// Expected output:
// 50
