// Level 3: Main application
// Located in: nested/test_nested_imports.mt
// Imports from: models/Shape.mt (which imports from utils/MathCore.mt)

import { Rectangle, Circle, compareAreas, normalizeValue, Drawable } from "../models/Shape.mt";

function main() : void {
    // Test Rectangle class (from Level 2)
    Rectangle rect = new Rectangle();
    rect.width = 5;
    rect.height = 10;
    int rectArea = rect.area();
    int rectPerimeter = rect.perimeter();
    int rectDiagonal = rect.diagonal();

    print("Rectangle - Width: 5, Height: 10");
    print("Area: " + rectArea);
    print("Perimeter: " + rectPerimeter);
    print("Diagonal^2: " + rectDiagonal);

    // Test Circle class (from Level 2)
    Circle circle = new Circle();
    circle.radius = 7;
    int circleArea = circle.area();
    int circleCircumference = circle.circumference();

    print("Circle - Radius: 7");
    print("Area: " + circleArea);
    print("Circumference: " + circleCircumference);

    // Test imported functions (from Level 2, which use Level 1)
    int largerArea = compareAreas(rectArea, circleArea);
    print("Larger area: " + largerArea);

    int normalized = normalizeValue(-42);
    print("Normalized -42: " + normalized);

    print("Multi-level import test completed!");
}

main();
