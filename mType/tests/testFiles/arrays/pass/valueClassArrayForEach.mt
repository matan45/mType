// Edge Case: Array of value class instances iterated with for-each

value class Pixel {
    private int r;
    private int g;
    private int b;

    public constructor(int r, int g, int b) {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    public function getR(): int { return this.r; }
    public function getG(): int { return this.g; }
    public function getB(): int { return this.b; }

    public function brightness(): int {
        return (this.r + this.g + this.b) / 3;
    }

    public function toString(): string {
        return "rgb(" + this.r + "," + this.g + "," + this.b + ")";
    }
}

function main(): void {
    print("=== Edge: Value Class Array For-Each ===");

    // Create array of value class
    Pixel[] pixels = [
        new Pixel(255, 0, 0),
        new Pixel(0, 255, 0),
        new Pixel(0, 0, 255),
        new Pixel(128, 128, 128),
        new Pixel(255, 255, 255)
    ];

    // For-each over value class array
    print("--- All pixels ---");
    for (Pixel p : pixels) {
        print(p.toString() + " brightness=" + p.brightness());
    }

    // Filter bright pixels via for-each
    print("--- Bright pixels (avg > 100) ---");
    int brightCount = 0;
    for (Pixel p : pixels) {
        if (p.brightness() > 100) {
            print(p.toString());
            brightCount = brightCount + 1;
        }
    }
    print("Count: " + brightCount);

    // Empty array for-each (should not crash)
    print("--- Empty array ---");
    Pixel[] empty = new Pixel[0];
    for (Pixel p : empty) {
        print("Should not print");
    }
    print("Empty iteration completed");

    // Single-element array
    print("--- Single element ---");
    Pixel[] single = [new Pixel(42, 42, 42)];
    for (Pixel p : single) {
        print("Single: " + p.toString());
    }

    // Nested for-each: value class array inside another loop
    print("--- Nested iteration ---");
    int[][] channels = [[255, 0, 0], [0, 255, 0]];
    for (int i = 0; i < channels.length; i++) {
        Pixel rowPixel = new Pixel(channels[i][0], channels[i][1], channels[i][2]);
        print("Row " + i + ": " + rowPixel.toString());
    }

    print("=== Edge Complete ===");
}

main();
