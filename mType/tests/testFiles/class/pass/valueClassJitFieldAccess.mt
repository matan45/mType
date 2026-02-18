// Test: JIT compilation of value class field access
// Exercises: jit_get_field_ic, jit_set_field_ic with ValueObject

value class Color {
    public int r;
    public int g;
    public int b;

    public constructor(int r, int g, int b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    public function toString(): string {
        return "rgb(" + this.r + "," + this.g + "," + this.b + ")";
    }
}

// Hot function: field read on value object returning string (BOXED path)
function describeColor(Color c): string {
    return c.toString();
}

// Hot function: pure int arithmetic (JIT-safe)
function computeBrightness(int r, int g, int b): int {
    return (r + g + b) / 3;
}

// === Test 1: Hot toString on value objects ===
Color white = new Color(255, 255, 255);
Color black = new Color(0, 0, 0);
string lastDesc = "";
for (int i = 0; i < 200; i = i + 1) {
    if (i % 2 == 0) {
        lastDesc = describeColor(white);
    } else {
        lastDesc = describeColor(black);
    }
}
print("last desc: " + lastDesc);

// === Test 2: Field access in interpreter, int arithmetic in JIT ===
int totalBrightness = 0;
Color mid = new Color(100, 150, 200);
for (int i = 0; i < 200; i = i + 1) {
    totalBrightness = totalBrightness + computeBrightness(mid.r, mid.g, mid.b);
}
// 200 * (100+150+200)/3 = 200 * 150 = 30000
print("total brightness: " + totalBrightness);

// === Test 3: Value class construction in hot loop ===
string lastColor = "";
for (int i = 0; i < 200; i = i + 1) {
    Color c = new Color(i, i * 2, i * 3);
    lastColor = describeColor(c);
}
print("last color: " + lastColor);

// === Test 4: Mixed regular object and value object ===
class Container {
    public Color color;
    public string label;

    constructor(Color c, string l) {
        this.color = c;
        this.label = l;
    }

    public function describe(): string {
        return this.label + ":" + this.color.toString();
    }
}

function getDescription(Container cont): string {
    return cont.describe();
}

Container box = new Container(new Color(10, 20, 30), "box");
string lastLabel = "";
for (int i = 0; i < 200; i = i + 1) {
    lastLabel = getDescription(box);
}
print("container: " + lastLabel);

print("Value class JIT field access tests passed!");
