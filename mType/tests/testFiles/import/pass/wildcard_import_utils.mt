// Test file for wildcard import system
// All public symbols should be importable with import *

public class Point {
    public int x;
    public int y;

    function distance() : int {
        return this.x * this.x + this.y * this.y;
    }
}

public class Vector {
    public float x;
    public float y;

    function magnitude() : float {
        return this.x * this.x + this.y * this.y;
    }
}

public function createPoint(int x, int y) : Point {
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
