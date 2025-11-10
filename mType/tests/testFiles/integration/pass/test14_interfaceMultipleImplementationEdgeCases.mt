// Integration Test 14: Interface Multiple Implementation Edge Cases
// Tests: Multiple interfaces + Method conflicts + Covariant returns

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Base interfaces
interface Drawable {
    function draw(): void;
    function getColor(): string;
}

interface Moveable {
    function move(int x, int y): void;
    function getPosition(): string;
}

interface Resizable {
    function resize(float factor): void;
    function getSize(): float;
}

// Combined interface
interface GameObject extends Drawable, Moveable {
    function update(): void;
}

// Class implementing multiple interfaces
class Sprite implements Drawable, Moveable, Resizable {
    private int x;
    private int y;
    private float size;
    private string color;

    constructor(int posX, int posY, float s, string c) {
        this.x = posX;
        this.y = posY;
        this.size = s;
        this.color = c;
    }

    public function draw(): void {
        print("Drawing sprite at (" + this.x + "," + this.y + ") color=" + this.color);
    }

    public function getColor(): string {
        return this.color;
    }

    public function move(int newX, int newY): void {
        this.x = newX;
        this.y = newY;
        print("Moved to (" + this.x + "," + this.y + ")");
    }

    public function getPosition(): string {
        return "(" + this.x + "," + this.y + ")";
    }

    public function resize(float factor): void {
        this.size = this.size * factor;
        print("Resized to " + this.size);
    }

    public function getSize(): float {
        return this.size;
    }
}

// Class implementing combined interface
class Player implements GameObject {
    private int x;
    private int y;
    private string name;

    constructor(string n, int posX, int posY) {
        this.name = n;
        this.x = posX;
        this.y = posY;
    }

    public function draw(): void {
        print("Drawing player " + this.name);
    }

    public function getColor(): string {
        return "blue";
    }

    public function move(int newX, int newY): void {
        this.x = newX;
        this.y = newY;
        print(this.name + " moved");
    }

    public function getPosition(): string {
        return "Player at (" + this.x + "," + this.y + ")";
    }

    public function update(): void {
        print("Updating player " + this.name);
    }

    public function getName(): string {
        return this.name;
    }
}

// Test covariant return types
class Base {
    public function create(): Base {
        return new Base();
    }

    public function getType(): string {
        return "Base";
    }
}

class Derived extends Base {
    private int value;

    constructor(int v) {
        this.value = v;
    }

    // Covariant return type
    @Override
    public function create(): Derived {
        return new Derived(100);
    }

    @Override
    public function getType(): string {
        return "Derived";
    }

    public function getValue(): int {
        return this.value;
    }
}

// Interface with generic methods
interface Processor<T> {
    function process(T input): T;
    function validate(T input): bool;
}

// Multiple implementations of same interface
class IntProcessor implements Processor<Int> {
    public function process(Int input): Int {
        return input * new Int(2);
    }

    public function validate(Int input): bool {
        return input.getValue() > 0;
    }
}

class StringProcessor implements Processor<String> {
    public function process(String input): String {
        return input + new String(" [processed]");
    }

    public function validate(String input): bool {
        return input.length() > 0;
    }
}

// Polymorphic interface test
function testDrawable(Drawable obj): void {
    obj.draw();
    print("Color: " + obj.getColor());
}

function testMoveable(Moveable obj): void {
    print("Position: " + obj.getPosition());
    obj.move(100, 200);
}

// Main test execution
print("=== Test 14: Interface Multiple Implementation Edge Cases ===");

// Test 1: Class with multiple interfaces
print("--- Test 1: Multiple interfaces ---");
Sprite sprite = new Sprite(10, 20, 50.0, "red");

Drawable drawable = sprite;
testDrawable(drawable);

Moveable moveable = sprite;
testMoveable(moveable);

Resizable resizable = sprite;
resizable.resize(1.5);
print("New size: " + resizable.getSize());

// Test 2: Combined interface
print("--- Test 2: Combined interface ---");
Player player = new Player("Hero", 0, 0);

GameObject gameObj = player;
gameObj.draw();
gameObj.move(50, 50);
gameObj.update();

Drawable drawablePlayer = player;
testDrawable(drawablePlayer);

// Test 3: Covariant return types
print("--- Test 3: Covariant returns ---");
Base base = new Base();
Base createdBase = base.create();
print("Created type: " + createdBase.getType());

Derived derived = new Derived(42);
Derived createdDerived = derived.create();
print("Created type: " + createdDerived.getType());
print("Created value: " + createdDerived.getValue());

// Polymorphic call with covariant return
Base polyRef = derived;
Base polyCreated = polyRef.create();
print("Polymorphic created type: " + polyCreated.getType());

// Test 4: Generic interface implementations
print("--- Test 4: Generic interfaces ---");
IntProcessor intProc = new IntProcessor();
Int intInput = new Int(5);

if (intProc.validate(intInput)) {
    Int intResult = intProc.process(intInput);
    print("Int processed: " + intResult.getValue());
}

StringProcessor strProc = new StringProcessor();
String strInput = new String("Test");

if (strProc.validate(strInput)) {
    String strResult = strProc.process(strInput);
    print("String processed: " + strResult.getValue());
}

// Test 5: Interface type checking
print("--- Test 5: Interface type checking ---");
print("sprite isClassOf Drawable: " + (sprite isClassOf Drawable));
print("sprite isClassOf Moveable: " + (sprite isClassOf Moveable));
print("sprite isClassOf Resizable: " + (sprite isClassOf Resizable));
print("player isClassOf GameObject: " + (player isClassOf GameObject));
print("player isClassOf Drawable: " + (player isClassOf Drawable));
print("player isClassOf Resizable: " + (player isClassOf Resizable));

// Test 6: Interface casting
print("--- Test 6: Interface casting ---");
Drawable d = sprite;
if (d isClassOf Moveable) {
    Moveable m = (Moveable)d;
    print("Successfully cast Drawable to Moveable");
    m.move(150, 150);
}

if (d isClassOf Resizable) {
    Resizable r = (Resizable)d;
    print("Successfully cast Drawable to Resizable");
    r.resize(2.0);
}

// Test 7: Interface arrays
print("--- Test 7: Interface arrays ---");
Drawable[] drawables = new Drawable[3];
drawables[0] = new Sprite(1, 1, 10.0, "blue");
drawables[1] = new Sprite(2, 2, 20.0, "green");
drawables[2] = new Player("NPC", 3, 3);

for (int i = 0; i < drawables.length; i = i + 1) {
    print("Drawing object " + i + ":");
    drawables[i].draw();
}

print("=== Test 14 Complete ===");
