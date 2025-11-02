// Test abstract class implements partial interface
// @Script

interface Drawable {
    func draw(): void;
    func getColor(): String;
    func getSize(): Int;
}

abstract class Shape implements Drawable {
    var size: Int;

    func init(size: Int) {
        this.size = size;
    }

    // Implement some interface methods
    func getSize(): Int {
        return this.size;
    }

    // Leave draw() and getColor() for subclasses
}

class Circle extends Shape {
    var color: String;

    func init(size: Int, color: String) {
        super.init(size);
        this.color = color;
    }

    func draw(): void {
        print("Drawing circle");
    }

    func getColor(): String {
        return this.color;
    }
}

class Rectangle extends Shape {
    var color: String;

    func init(size: Int, color: String) {
        super.init(size);
        this.color = color;
    }

    func draw(): void {
        print("Drawing rectangle");
    }

    func getColor(): String {
        return this.color;
    }
}

var circle = new Circle(10, "red");
circle.draw();
print(circle.getColor());
print(circle.getSize());

var rect = new Rectangle(20, "blue");
rect.draw();
print(rect.getColor());
print(rect.getSize());
