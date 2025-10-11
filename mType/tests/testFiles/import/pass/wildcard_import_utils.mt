// Test file for wildcard import system
// All public symbols should be importable with import *

public class Point {
    int x;
    int y;

    function distance() : float {
        return sqrt(float(this.x * this.x + this.y * this.y));
    }
}

public class Vector {
    float x;
    float y;

    function magnitude() : float {
        return sqrt(this.x * this.x + this.y * this.y);
    }
}

public function createPoint(int x, int y) : object {
    Point p = new Point();
    p.x = x;
    p.y = y;
    return p;
}

private function secretFunction() : int {
    return 99;
}

private class SecretClass {
    int hidden;
}
