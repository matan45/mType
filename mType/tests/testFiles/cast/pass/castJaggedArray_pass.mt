// Test jagged array casting
// Demonstrates type casting with arrays of varying lengths

class Shape {
    string type;
}

class Rectangle extends Shape {
    int width;
    int height;

    constructor() {
        type = "Rectangle";
    }
}

@Script
function testJaggedArrayCasting(): void {
    print("Testing jagged array casting");

    // Create jagged array with different row lengths
    Rectangle[][] jagged = new Rectangle[3][];

    // First row has 2 elements
    jagged[0] = new Rectangle[2];
    jagged[0][0] = new Rectangle();
    jagged[0][0].width = 10;
    jagged[0][0].height = 20;
    jagged[0][1] = new Rectangle();
    jagged[0][1].width = 15;
    jagged[0][1].height = 25;

    // Second row has 3 elements
    jagged[1] = new Rectangle[3];
    jagged[1][0] = new Rectangle();
    jagged[1][0].width = 30;
    jagged[1][1] = new Rectangle();
    jagged[1][1].width = 40;
    jagged[1][2] = new Rectangle();
    jagged[1][2].width = 50;

    // Third row has 1 element
    jagged[2] = new Rectangle[1];
    jagged[2][0] = new Rectangle();
    jagged[2][0].width = 100;
    jagged[2][0].height = 200;

    print("Jagged array rows: " + jagged.length);
    print("Row 0 length: " + jagged[0].length);
    print("Row 1 length: " + jagged[1].length);
    print("Row 2 length: " + jagged[2].length);

    // Upcast to Shape jagged array
    Shape[][] shapeJagged = jagged;
    print("Shape at [0][0] type: " + shapeJagged[0][0].type);
    print("Shape at [2][0] type: " + shapeJagged[2][0].type);

    // Cast back individual elements
    Rectangle rect = (Rectangle)shapeJagged[0][0];
    print("Rectangle [0][0] dimensions: " + rect.width + "x" + rect.height);

    rect = (Rectangle)shapeJagged[1][2];
    print("Rectangle [1][2] width: " + rect.width);

    rect = (Rectangle)shapeJagged[2][0];
    print("Rectangle [2][0] dimensions: " + rect.width + "x" + rect.height);

    print("Jagged array casting completed");
}
