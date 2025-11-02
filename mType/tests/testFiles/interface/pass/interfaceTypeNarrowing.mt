// Test type narrowing with multiple interfaces
// @Script

interface Drawable {
    func draw(): void;
}

interface Colorable {
    func setColor(color: String): void;
    func getColor(): String;
}

interface Resizable {
    func resize(factor: Int): void;
}

class SimpleShape implements Drawable {
    func draw(): void {
        print("Drawing simple shape");
    }
}

class ColoredShape implements Drawable, Colorable {
    var color: String;

    func init() {
        this.color = "white";
    }

    func draw(): void {
        print("Drawing " + this.color + " shape");
    }

    func setColor(color: String): void {
        this.color = color;
    }

    func getColor(): String {
        return this.color;
    }
}

class AdvancedShape implements Drawable, Colorable, Resizable {
    var color: String;
    var size: Int;

    func init() {
        this.color = "white";
        this.size = 10;
    }

    func draw(): void {
        print("Drawing " + this.color + " shape, size: " + this.size.toString());
    }

    func setColor(color: String): void {
        this.color = color;
    }

    func getColor(): String {
        return this.color;
    }

    func resize(factor: Int): void {
        this.size = this.size * factor;
    }
}

func processShape(shape: Drawable): void {
    shape.draw();

    // Type narrowing - check for additional capabilities
    if (shape instanceof Colorable) {
        var colorable: Colorable = shape as Colorable;
        colorable.setColor("red");
        print("Set color to: " + colorable.getColor());
    }

    if (shape instanceof Resizable) {
        var resizable: Resizable = shape as Resizable;
        resizable.resize(2);
        print("Resized shape");
    }

    // Draw again to see changes
    shape.draw();
}

processShape(new SimpleShape());
print("---");
processShape(new ColoredShape());
print("---");
processShape(new AdvancedShape());
