// Level 2: Shape models that use MathCore
// Located in: models/Shape.mt
// Imports from: utils/MathCore.mt

import { MathCore, abs, max } from "../utils/MathCore.mt"

public class Rectangle {
    int width;
    int height;

    function area() : int {
        return this.width * this.height;
    }

    function perimeter() : int {
        return 2 * (this.width + this.height);
    }

    function diagonal() : int {
        // Use MathCore from imported file
        MathCore math = new MathCore();
        int widthSquared = math.square(this.width);
        int heightSquared = math.square(this.height);
        return widthSquared + heightSquared;
    }
}

public class Circle {
    int radius;

    function area() : int {
        // Use MathCore from imported file
        MathCore math = new MathCore();
        return math.square(this.radius) * 3; // Simplified PI approximation
    }

    function circumference() : int {
        return 2 * 3 * this.radius; // Simplified PI approximation
    }
}

public function compareAreas(int area1, int area2) : int {
    // Use imported max function
    return max(area1, area2);
}

public function normalizeValue(int value) : int {
    // Use imported abs function
    return abs(value);
}

private class InternalShape {
    int sides;
}

public interface Drawable {
    function draw() : void;
}
