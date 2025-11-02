// Test valid cross-cast when object implements multiple interfaces
// @Script

interface Drawable {
    func draw(): void;
}

interface Resizable {
    func resize(factor: Int): void;
}

class Shape implements Drawable, Resizable {
    var size: Int;

    func init(size: Int) {
        this.size = size;
    }

    func draw(): void {
        print("Drawing shape of size: " + this.size.toString());
    }

    func resize(factor: Int): void {
        this.size = this.size * factor;
        print("Resized to: " + this.size.toString());
    }
}

var drawable: Drawable = new Shape(10);
drawable.draw();

// Valid cross-cast - the underlying object implements both interfaces
var resizable: Resizable = drawable as Resizable;
resizable.resize(2);

// Cast back
var drawable2: Drawable = resizable as Drawable;
drawable2.draw();
