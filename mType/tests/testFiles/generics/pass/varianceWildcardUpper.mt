import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Wildcard upper bound (? extends T)
class Shape {
    String name;

    public function Shape(String n) {
        name = n;
    }

    public function getName(): String {
        return name;
    }
}

class Circle extends Shape {
    public function Circle() {
        super(new String("Circle"));
    }
}

class Rectangle extends Shape {
    public function Rectangle() {
        super(new String("Rectangle"));
    }
}

class Container<T extends Shape> {
    T item;

    public function setItem(T i): void {
        item = i;
    }

    public function getItem(): T {
        return item;
    }
}

function printShape(Container<Shape> container): void {
    Shape s = container.getItem();
    print(s.getName());
}

function main(): void {
    Container<Circle> circleContainer = new Container<Circle>();
    circleContainer.setItem(new Circle());

    // Upper bounded wildcard - can read any Shape
    printShape(circleContainer);

    Container<Rectangle> rectContainer = new Container<Rectangle>();
    rectContainer.setItem(new Rectangle());
    printShape(rectContainer);
}

main();
