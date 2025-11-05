// Test valid cross-cast when object implements multiple interfaces
// @Script

interface Drawable {
    function draw(): void;
}

interface Resizable {
    function resize(int factor): void;
}

class Shape implements Drawable, Resizable {
    private int size;

    public constructor(int size) {
        this.size = size;
    }

    public function draw(): void {
        print("Drawing shape of size: " + this.size);
    }

    public function resize(int factor): void {
        this.size = this.size * factor;
        print("Resized to: " + this.size);
    }
}

Drawable drawable = new Shape(10);
drawable.draw();

// Valid cross-cast - the underlying object implements both interfaces
Resizable resizable = (Resizable)drawable;
resizable.resize(2);

// Cast back
Drawable drawable2 = (Drawable)resizable;
drawable2.draw();
