// Class implementing multiple interfaces test
interface Drawable {
    function draw(): void;
}

interface Moveable {
    function move(float x, float y): void;
}

interface Resizable {
    function resize(float factor): void;
}

class GameObject implements Drawable, Moveable, Resizable {
    float x;
    float y;
    float width;
    float height;

    constructor(float w, float h) {
        this.x = 0.0;
        this.y = 0.0;
        this.width = w;
        this.height = h;
    }

    public function draw(): void {
        print("Drawing game object at (" + this.x + ", " + this.y + ")");
    }

    public function move(float newX, float newY): void {
        this.x = newX;
        this.y = newY;
    }

    public function resize(float factor): void {
        this.width = this.width * factor;
        this.height = this.height * factor;
    }
}

GameObject obj = new GameObject(10.0, 20.0);
obj.draw();
obj.move(5.0, 15.0);
obj.resize(1.5);
obj.draw();

print("Multiple interface implementation successful");