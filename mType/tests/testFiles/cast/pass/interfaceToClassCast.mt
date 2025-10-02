// Test: Cast interface back to class
interface Shape {
    void area();
}

class Rectangle implements Shape {
    int width = 5;
    int height = 10;

    void area() {
        print(this.width * this.height);
    }
}

Rectangle rect = new Rectangle();
Shape shape = rect;
Rectangle castedRect = (Rectangle)shape;
castedRect.area();

// Expected output:
// 50
