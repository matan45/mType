// Test casting array of interface type to array of implementation type
// This demonstrates array type compatibility with interfaces

interface Drawable {
    void draw();
}

class Circle extends Drawable {
    int radius;

    void draw() {
        print("Drawing circle with radius: " + radius);
    }
}

class Square extends Drawable {
    int side;

    void draw() {
        print("Drawing square with side: " + side);
    }
}

@Script
void testInterfaceArrayCasting() {
    print("Testing interface array casting");

    // Create array of Circle implementations
    Circle[] circles = new Circle[2];
    circles[0] = new Circle();
    circles[0].radius = 5;
    circles[1] = new Circle();
    circles[1].radius = 10;

    // Upcast to Drawable array
    Drawable[] drawables = circles;
    print("Array length: " + drawables.length);

    // Call interface methods
    drawables[0].draw();
    drawables[1].draw();

    // Check if we can safely downcast back
    if (drawables isClassOf Circle[]) {
        Circle[] circlesAgain = (Circle[])drawables;
        print("Successfully downcast back to Circle[]");
        print("First circle radius: " + circlesAgain[0].radius);
    }

    print("Interface array casting test completed");
}
