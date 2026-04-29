// Combo 21: Pattern Match + Annotation Reflection
// Tests: annotated Shape subclasses; match on subtype + reflect annotation per branch

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";

@Retention(RUNTIME)
@Target([CLASS])
annotation Component {
    string name = "";
}

abstract class Shape {
    public abstract function name(): string;
}

@Component(name = "circle-component")
class Circle extends Shape {
    private float radius;

    public constructor(float r) {
        this.radius = r;
    }

    public function getRadius(): float { return this.radius; }
    public function name(): string { return "Circle"; }
}

@Component(name = "square-component")
class Square extends Shape {
    private float side;

    public constructor(float s) {
        this.side = s;
    }

    public function getSide(): float { return this.side; }
    public function name(): string { return "Square"; }
}

@Component(name = "triangle-component")
class Triangle extends Shape {
    private float base;

    public constructor(float b) {
        this.base = b;
    }

    public function getBase(): float { return this.base; }
    public function name(): string { return "Triangle"; }
}

function readComponentName(string className): string {
    Class cls = Class::forName(className);
    if (cls.hasAnnotation("Component")) {
        Annotation? ann = cls.getAnnotation("Component");
        if (ann != null) {
            return ann.getString("name");
        }
    }
    return "<none>";
}

function main(): void {
    print("=== Combo 21: Pattern Match + Annotation ===");

    Shape[] shapes = [
        new Circle(2.0),
        new Square(3.0),
        new Triangle(4.0),
        new Circle(5.0)
    ];

    for (int i = 0; i < shapes.length; i++) {
        Shape s = shapes[i];
        match (s) {
            case Circle c -> {
                print("Circle r=" + c.getRadius() + " ann=" + readComponentName("Circle"));
            }
            case Square sq -> {
                print("Square side=" + sq.getSide() + " ann=" + readComponentName("Square"));
            }
            case Triangle t -> {
                print("Triangle base=" + t.getBase() + " ann=" + readComponentName("Triangle"));
            }
            default -> {
                print("Unknown");
            }
        }
    }

    print("=== Combo 21 Complete ===");
}

main();
