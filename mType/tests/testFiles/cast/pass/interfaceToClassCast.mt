// Test: Cast interface back to class
interface Shape {
    function area(): void;
}

class Rectangle implements Shape {
    int width = 5;
    int height = 10;

    function area(): void {
        print(this.width * this.height);
    }
}

Rectangle rect = new Rectangle();
Shape shape = rect;
Rectangle castedRect = (Rectangle)shape;
castedRect.area();

// Expected output:
// 50
