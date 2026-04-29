// Combo 05: Switch + Pattern Matching + Casting in Loops
// Tests: Iterate over heterogeneous array, switch with pattern matching + casting

abstract class Shape {
    abstract function area(): float;
    abstract function name(): string;
}

class Circle extends Shape {
    private float radius;
    public constructor(float r) { this.radius = r; }
    public function area(): float { return 3.14 * this.radius * this.radius; }
    public function name(): string { return "Circle"; }
    public function getRadius(): float { return this.radius; }
}

class Rectangle extends Shape {
    private float w;
    private float h;
    public constructor(float w, float h) { this.w = w; this.h = h; }
    public function area(): float { return this.w * this.h; }
    public function name(): string { return "Rectangle"; }
    public function getWidth(): float { return this.w; }
    public function getHeight(): float { return this.h; }
}

class Triangle extends Shape {
    private float base;
    private float height;
    public constructor(float b, float h) { this.base = b; this.height = h; }
    public function area(): float { return 0.5 * this.base * this.height; }
    public function name(): string { return "Triangle"; }
}

function classifyByArea(float area): string {
    // Switch on computed category
    int category = 0;
    if (area < 10.0) {
        category = 1;
    } else if (area < 100.0) {
        category = 2;
    } else {
        category = 3;
    }

    string result = "";
    switch (category) {
        case 1:
            result = "small";
            break;
        case 2:
            result = "medium";
            break;
        case 3:
            result = "large";
            break;
        default:
            result = "unknown";
            break;
    }
    return result;
}

function main(): void {
    print("=== Combo 05: Switch + Pattern Match + Cast in Loops ===");

    Shape[] shapes = [
        new Circle(1.0),
        new Rectangle(5.0, 20.0),
        new Triangle(6.0, 4.0),
        new Circle(10.0),
        new Rectangle(2.0, 3.0)
    ];

    // Loop with pattern matching + casting
    print("--- Pattern match in loop ---");
    for (int i = 0; i < shapes.length; i++) {
        Shape s = shapes[i];
        float area = s.area();
        string size = classifyByArea(area);

        match (s) {
            case Circle c -> {
                print("Circle r=" + c.getRadius() + " area=" + area + " [" + size + "]");
            }
            case Rectangle r -> {
                print("Rect " + r.getWidth() + "x" + r.getHeight() + " area=" + area + " [" + size + "]");
            }
            case Triangle t -> {
                print("Triangle area=" + area + " [" + size + "]");
            }
            default -> {
                print("Unknown shape");
            }
        }
    }

    // Casting in loop with instanceof check
    print("--- Cast + filter in loop ---");
    int circleCount = 0;
    float totalCircleArea = 0.0;
    for (int i = 0; i < shapes.length; i++) {
        if (shapes[i] isClassOf Circle) {
            Circle c = (Circle)shapes[i];
            circleCount = circleCount + 1;
            totalCircleArea = totalCircleArea + c.area();
        }
    }
    print("Circles found: " + circleCount);
    print("Total circle area: " + totalCircleArea);

    // Switch on string from pattern-matched type
    print("--- Switch on shape name ---");
    for (int i = 0; i < shapes.length; i++) {
        string name = shapes[i].name();
        switch (name) {
            case "Circle":
                print(i + ": round shape");
                break;
            case "Rectangle":
                print(i + ": rectangular shape");
                break;
            case "Triangle":
                print(i + ": triangular shape");
                break;
            default:
                print(i + ": other");
                break;
        }
    }

    print("=== Combo 05 Complete ===");
}

main();
