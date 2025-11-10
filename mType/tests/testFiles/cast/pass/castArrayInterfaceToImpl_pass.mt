// Test casting array of interface type to array of implementation type
// This demonstrates array type compatibility with interfaces

interface Drawable {
    function draw(): void;
}

class Circle implements Drawable {
    public int radius;

    public function draw(): void {
        print("Drawing circle with radius: " + radius);
    }
}

class Square implements Drawable {
    int side;

    public function draw(): void {
        print("Drawing square with side: " + side);
    }
}


function testInterfaceArrayCasting(): void {
    print("Testing interface array casting");

    // Create array of Circle implementations
    Circle[] circles = new Circle[2];
    circles[0] = new Circle();
    circles[0].radius = 5;
    circles[1] = new Circle();
    circles[1].radius = 10;

    // Create Drawable array and assign elements
    Drawable[] drawables = new Drawable[2];
    drawables[0] = circles[0];
    drawables[1] = circles[1];
    print("Array length: " + drawables.length);

    // Call interface methods
    drawables[0].draw();
    drawables[1].draw();

    // Cast individual element back to Circle
    Circle firstCircle = (Circle)drawables[0];
    print("Successfully cast element to Circle");
    print("First circle radius: " + firstCircle.radius);

    print("Interface array casting test completed");
}
testInterfaceArrayCasting();
