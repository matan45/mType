// Test interface reflection

import * from "../../lib/reflect/Class.mt";

interface Drawable {
    function draw(): void;
}

interface Resizable {
    function resize(int width, int height): void;
}

class Shape implements Drawable, Resizable {
    public int width;
    public int height;

    public constructor() {
        this.width = 0;
        this.height = 0;
    }

    public function draw(): void {
        print("Drawing shape");
    }

    public function resize(int width, int height): void {
        this.width = width;
        this.height = height;
    }
}

Class shapeClass = Class::forName("Shape");

// Check if class is an interface (should be false)
print("Shape isInterface: " + shapeClass.isInterface());

// Get implemented interfaces
string[] interfaces = shapeClass.getInterfaces();
print("Number of interfaces: " + interfaces.length);

for (int i = 0; i < interfaces.length; i = i + 1) {
    print("  Implements: " + interfaces[i]);
}

print("Test passed");
