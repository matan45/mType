// Test: Value class with Object methods
// Expected: Pass - value classes inherit Object methods implicitly

value class Color {
    private int r;
    private int g;
    private int b;

    public constructor(int r, int g, int b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    public function getR(): int {
        return this.r;
    }

    public function getG(): int {
        return this.g;
    }

    public function getB(): int {
        return this.b;
    }

    public function toString(): string {
        return "rgb(" + this.r + ", " + this.g + ", " + this.b + ")";
    }

    public function equals(Color other): bool {
        return this.r == other.r && this.g == other.g && this.b == other.b;
    }

    public function hashCode(): int {
        return this.r * 31 * 31 + this.g * 31 + this.b;
    }
}

Color red = new Color(255, 0, 0);
Color blue = new Color(0, 0, 255);
Color red2 = new Color(255, 0, 0);

print("red: " + red.toString());
print("blue: " + blue.toString());
print("red equals red2: " + red.equals(red2));
print("red equals blue: " + red.equals(blue));
print("red hashCode: " + red.hashCode());
