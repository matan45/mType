// Test: match type patterns with inheritance and interfaces (isClassOf semantics)
// The type pattern uses INSTANCEOF which checks class hierarchy and interfaces

interface Drawable {
    function draw(): string;
}

interface Resizable {
    function resize(float factor): void;
}

abstract class Widget {
    abstract function getType(): string;
}

class Button extends Widget implements Drawable {
    public function getType(): string { return "Button"; }
    public function draw(): string { return "Drawing button"; }
}

class Slider extends Widget implements Drawable, Resizable {
    private float size;
    constructor() { this.size = 1.0; }
    public function getType(): string { return "Slider"; }
    public function draw(): string { return "Drawing slider (size=" + this.size + ")"; }
    public function resize(float factor): void { this.size = this.size * factor; }
    public function getSize(): float { return this.size; }
}

class Label extends Widget {
    public function getType(): string { return "Label"; }
}

// Match on concrete class types with variable binding
function describeWidget(Widget w): void {
    match (w) {
        case Slider s -> {
            print("Slider: " + s.draw());
            s.resize(2.0);
            print("After resize: " + s.getSize());
        }
        case Button b -> print("Button: " + b.draw());
        case Label l -> print("Label: " + l.getType());
        default -> print("Unknown widget");
    }
}

// Match on interface types
function tryDraw(Widget w): void {
    match (w) {
        case Drawable d -> print("Can draw: " + d.draw());
        default -> print("Not drawable");
    }
}

function main(): void {
    Widget btn = new Button();
    Widget sld = new Slider();
    Widget lbl = new Label();

    print("=== Concrete type matching ===");
    describeWidget(btn);
    describeWidget(sld);
    describeWidget(lbl);

    print("=== Interface type matching ===");
    tryDraw(btn);
    tryDraw(sld);
    tryDraw(lbl);
}
main();
