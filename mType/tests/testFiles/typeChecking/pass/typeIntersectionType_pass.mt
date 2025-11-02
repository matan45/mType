// Test: Intersection type behavior through multiple interfaces
// Expected: Pass - classes implementing multiple interfaces (intersection)

// First interface
interface Drawable {
    void draw();
    string getDescription();
}

// Second interface
interface Resizable {
    void resize(int width, int height);
    int getWidth();
    int getHeight();
}

// Third interface
interface Movable {
    void move(int x, int y);
    int getX();
    int getY();
}

// Class implementing single interface
class Circle implements Drawable {
    private int radius;

    public constructor(int r) {
        this.radius = r;
    }

    public void draw() {
        print("Drawing circle with radius: " + this.radius);
    }

    public string getDescription() {
        return "Circle";
    }

    public int getRadius() {
        return this.radius;
    }
}

// Class implementing two interfaces (intersection)
class Rectangle implements Drawable, Resizable {
    private int width;
    private int height;

    public constructor(int w, int h) {
        this.width = w;
        this.height = h;
    }

    public void draw() {
        print("Drawing rectangle: " + this.width + "x" + this.height);
    }

    public string getDescription() {
        return "Rectangle";
    }

    public void resize(int w, int h) {
        this.width = w;
        this.height = h;
        print("Resized to: " + w + "x" + h);
    }

    public int getWidth() {
        return this.width;
    }

    public int getHeight() {
        return this.height;
    }
}

// Class implementing all three interfaces (full intersection)
class Sprite implements Drawable, Resizable, Movable {
    private int width;
    private int height;
    private int x;
    private int y;
    private string name;

    public constructor(string name, int w, int h, int x, int y) {
        this.name = name;
        this.width = w;
        this.height = h;
        this.x = x;
        this.y = y;
    }

    public void draw() {
        print("Drawing sprite '" + this.name + "' at (" + this.x + "," + this.y + ") size " + this.width + "x" + this.height);
    }

    public string getDescription() {
        return "Sprite: " + this.name;
    }

    public void resize(int w, int h) {
        this.width = w;
        this.height = h;
    }

    public int getWidth() {
        return this.width;
    }

    public int getHeight() {
        return this.height;
    }

    public void move(int newX, int newY) {
        this.x = newX;
        this.y = newY;
        print("Moved to: (" + newX + "," + newY + ")");
    }

    public int getX() {
        return this.x;
    }

    public int getY() {
        return this.y;
    }
}

// Test 1: Single interface usage
print("Test 1: Single interface");
Drawable d1 = new Circle(5);
d1.draw();
print("Description: " + d1.getDescription());

// Test 2: Intersection of two interfaces
print("\nTest 2: Two interface intersection");
Rectangle rect = new Rectangle(10, 20);
Drawable d2 = rect;  // Can be treated as Drawable
d2.draw();

Resizable r1 = rect;  // Can be treated as Resizable
r1.resize(15, 25);
print("New dimensions: " + r1.getWidth() + "x" + r1.getHeight());

// Test 3: Full intersection (all three interfaces)
print("\nTest 3: Three interface intersection");
Sprite sprite = new Sprite("Hero", 32, 32, 0, 0);

Drawable d3 = sprite;
d3.draw();

Resizable r2 = sprite;
r2.resize(64, 64);

Movable m1 = sprite;
m1.move(100, 200);

sprite.draw();  // Show final state

// Test 4: Array of intersection types
print("\nTest 4: Array of drawable & resizable");
Rectangle[] rectangles = new Rectangle[2];
rectangles[0] = new Rectangle(5, 10);
rectangles[1] = new Rectangle(15, 20);

int i = 0;
while (i < 2) {
    print("\nRectangle " + i + ":");
    Drawable d = rectangles[i];
    d.draw();

    Resizable r = rectangles[i];
    r.resize(r.getWidth() * 2, r.getHeight() * 2);

    rectangles[i].draw();
    i = i + 1;
}

// Test 5: Function accepting intersection type
function drawAndResize(Drawable d, Resizable r): void {
    print("Drawing:");
    d.draw();
    print("Resizing:");
    r.resize(100, 100);
}

print("\nTest 5: Function with intersection parameters");
Rectangle testRect = new Rectangle(30, 40);
drawAndResize(testRect, testRect);
testRect.draw();

// Test 6: Complex intersection with sprite
function updateSprite(Drawable d, Resizable r, Movable m): void {
    print("Updating sprite:");
    d.draw();
    m.move(50, 75);
    r.resize(48, 48);
}

print("\nTest 6: Full intersection function");
Sprite testSprite = new Sprite("Enemy", 16, 16, 10, 10);
updateSprite(testSprite, testSprite, testSprite);
testSprite.draw();

print("\nIntersection type tests completed");
