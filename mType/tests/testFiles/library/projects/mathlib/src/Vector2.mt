// MathLib: 2D vector class

class Vector2 {
    public float x;
    public float y;

    public constructor(float x, float y) {
        this.x = x;
        this.y = y;
    }

    public constructor() {
        this.x = 0.0;
        this.y = 0.0;
    }

    public function add(Vector2 other): Vector2 {
        return new Vector2(this.x + other.x, this.y + other.y);
    }

    public function scale(float factor): Vector2 {
        return new Vector2(this.x * factor, this.y * factor);
    }

    public function dot(Vector2 other): float {
        return this.x * other.x + this.y * other.y;
    }

    public function toString(): string {
        return "Vector2(" + this.x + ", " + this.y + ")";
    }
}
