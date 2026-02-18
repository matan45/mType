// Test: Value class with instance and static methods
// Expected: Pass - methods work on value classes

value class Vector2D {
    private float x;
    private float y;

    public constructor(float x, float y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): float {
        return this.x;
    }

    public function getY(): float {
        return this.y;
    }

    public function add(Vector2D other): Vector2D {
        return new Vector2D(this.x + other.x, this.y + other.y);
    }

    public function scale(float factor): Vector2D {
        return new Vector2D(this.x * factor, this.y * factor);
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }

    public static function zero(): Vector2D {
        return new Vector2D(0.0, 0.0);
    }
}

// Instance methods
Vector2D v1 = new Vector2D(1.0, 2.0);
Vector2D v2 = new Vector2D(3.0, 4.0);
print("v1: " + v1.toString());
print("v2: " + v2.toString());

// Add returns new value
Vector2D v3 = v1.add(v2);
print("v1 + v2: " + v3.toString());

// Scale returns new value
Vector2D v4 = v1.scale(2.0);
print("v1 * 2: " + v4.toString());

// Static method
Vector2D v5 = Vector2D.zero();
print("zero: " + v5.toString());

// Original unchanged
print("v1 after ops: " + v1.toString());
