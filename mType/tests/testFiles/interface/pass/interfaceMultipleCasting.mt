// Test casting same object to different interfaces it implements

interface Drawable {
    function draw(): string;
}

interface Moveable {
    function move(int x, int y): string;
}

interface Resizable {
    function resize(int width, int height): string;
}

class GameObject implements Drawable, Moveable, Resizable {
    private int x;
    private int y;
    private int width;
    private int height;

    public constructor() {
        this.x = 0;
        this.y = 0;
        this.width = 100;
        this.height = 100;
    }

    public function draw(): string {
        return "Drawing at (" + this.x + ", " + this.y + ") size " + this.width + "x" + this.height;
    }

    public function move(int newX, int newY): string {
        this.x = newX;
        this.y = newY;
        return "Moved to (" + this.x + ", " + this.y + ")";
    }

    public function resize(int w, int h): string {
        this.width = w;
        this.height = h;
        return "Resized to " + this.width + "x" + this.height;
    }
}

function main(): void {
    print("Testing multiple interface casting");

    GameObject obj = new GameObject();

    // Use as GameObject
    print("Initial: " + obj.draw());

    // Cast to Drawable
    Drawable drawable = (Drawable)obj;
    print("As Drawable: " + drawable.draw());

    // Cast to Moveable
    Moveable moveable = (Moveable)obj;
    print("As Moveable: " + moveable.move(50, 75));

    // Cast to Resizable
    Resizable resizable = (Resizable)obj;
    print("As Resizable: " + resizable.resize(200, 150));

    // All interfaces refer to same object
    print("After changes: " + drawable.draw());

    // Cast back to GameObject
    GameObject obj2 = (GameObject)drawable;
    print("Cast back: " + obj2.draw());

    print("Multiple interface casting test completed");
}

main();
