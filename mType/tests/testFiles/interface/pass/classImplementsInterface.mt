// Class implementing interface test
interface Drawable {
    function draw(): void;
    function getArea(): float;
}

class Rectangle implements Drawable {
    private float width;
    private float height;

    public constructor(float w, float h) {
        this.width = w;
        this.height = h;
    }

    public function draw(): void {
        print("Drawing rectangle");
    }

    public function getArea(): float {
        return this.width * this.height;
    }
}

Rectangle rect = new Rectangle(5.0, 3.0);
rect.draw();
print("Area: " + rect.getArea());
print("Class implements interface successful");
