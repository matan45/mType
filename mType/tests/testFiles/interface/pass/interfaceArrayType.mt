// Test interface array type
// @Script

import * from "../../lib/collections/List.mt";

interface Drawable {
    function draw(): void;
}

class Circle implements Drawable {
    private int radius;

    public constructor(int radius) {
        this.radius = radius;
    }

    public function draw(): void {
        print("Drawing circle with radius: " + this.radius);
    }
}

class Rectangle implements Drawable {
    private int width;
    private int height;

    public constructor(int width, int height) {
        this.width = width;
        this.height = height;
    }

    public function draw(): void {
        print("Drawing rectangle " + this.width + "x" + this.height);
    }
}

// Array of interface type
List<Drawable> shapes = new List<Drawable>();
shapes.add(new Circle(5));
shapes.add(new Rectangle(10, 20));
shapes.add(new Circle(15));

// Draw all shapes
for (int i = 0; i < shapes.size(); i++) {
    Drawable shape = shapes.get(i);
    shape.draw();
}
