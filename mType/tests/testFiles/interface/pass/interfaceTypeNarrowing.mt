// Test type narrowing with multiple interfaces
// @Script

interface Drawable {
    function draw(): void;
}

interface Colorable {
    function setColor(string color): void;
    function getColor(): string;
}

interface Resizable {
    function resize(int factor): void;
}

class SimpleShape implements Drawable {
    public function draw(): void {
        print("Drawing simple shape");
    }
}

class ColoredShape implements Drawable, Colorable {
    private string color;

    public constructor() {
        this.color = "white";
    }

    public function draw(): void {
        print("Drawing " + this.color + " shape");
    }

    public function setColor(string color): void {
        this.color = color;
    }

    public function getColor(): string {
        return this.color;
    }
}

class AdvancedShape implements Drawable, Colorable, Resizable {
    private string color;
    private int size;
    private bool resized;

    public constructor() {
        this.color = "white";
        this.size = 10;
        this.resized = false;
    }

    public function draw(): void {
        if (this.resized) {
            print("Drawing " + this.color + " shape, size: " + this.size);
        } else {
            print("Drawing " + this.color + " shape");
        }
    }

    public function setColor(string color): void {
        this.color = color;
    }

    public function getColor(): string {
        return this.color;
    }

    public function resize(int factor): void {
        this.size = this.size * factor;
        this.resized = true;
    }
}

function processShape(Drawable shape): void {
    shape.draw();

    // Type narrowing - check for additional capabilities
    if (shape isClassOf Colorable) {
        Colorable colorable = (Colorable)shape;
        colorable.setColor("red");
        print("Set color to: " + colorable.getColor());
    }

    if (shape isClassOf Resizable) {
        Resizable resizable = (Resizable)shape;
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
