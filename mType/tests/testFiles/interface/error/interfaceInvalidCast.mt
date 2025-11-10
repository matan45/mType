// Test invalid cast from one interface to unrelated interface
// @Throw

interface Drawable {
    func draw(): void;
}

interface Serializable {
    func serialize(): String;
}

class Circle implements Drawable {
    var radius: Int;

    func init(radius: Int) {
        this.radius = radius;
    }

    func draw(): void {
        print("Drawing circle");
    }
}

var drawable: Drawable = new Circle(5);

// Error: Cannot cast Drawable to Serializable - they are unrelated
var serializable: Serializable = drawable as Serializable;
print(serializable.serialize());
