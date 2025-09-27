// Incomplete interface implementation error test
interface Drawable {
    function draw(): void;
    function getArea(): float;
}

class Rectangle implements Drawable {
    float width;
    float height;

    constructor(float w, float h) {
        this.width = w;
        this.height = h;
    }

    // Missing getArea() method implementation
    function draw(): void {
        print("Drawing rectangle");
    }
}

Rectangle rect = new Rectangle(5.0, 3.0);
print("This should not print");