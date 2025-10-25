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
    int x;
    int y;
    int width;
    int height;

    constructor() {
        x = 0;
        y = 0;
        width = 100;
        height = 100;
    }

    public function draw(): string {
        return "Drawing at (" + x + ", " + y + ") size " + width + "x" + height;
    }

    public function move(int newX, int newY): string {
        x = newX;
        y = newY;
        return "Moved to (" + x + ", " + y + ")";
    }

    public function resize(int w, int h): string {
        width = w;
        height = h;
        return "Resized to " + width + "x" + height;
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
