// Test: Interfaces in standalone exe

interface Drawable {
    function draw(): string;
}

interface Resizable {
    function resize(float factor): void;
}

class Widget implements Drawable, Resizable {
    private string name;
    private float size;

    public constructor(string n, float s) {
        this.name = n;
        this.size = s;
    }

    public function draw(): string {
        return "Drawing " + this.name + " at size " + this.size;
    }

    public function resize(float factor): void {
        this.size = this.size * factor;
    }

    public function getSize(): float {
        return this.size;
    }
}

class Label implements Drawable {
    private string text;

    public constructor(string t) {
        this.text = t;
    }

    public function draw(): string {
        return "Label: " + this.text;
    }
}

function renderDrawable(Drawable d): void {
    print(d.draw());
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Single interface implementation
        Label label = new Label("Hello World");
        print(label.draw());

        // Multiple interface implementation
        Widget widget = new Widget("Button", 10.0);
        print(widget.draw());

        widget.resize(2.0);
        print("After resize: " + widget.getSize());

        // Interface as parameter type
        renderDrawable(label);
        renderDrawable(widget);

        print("Interfaces test passed");
    }
}
