// Level 1: Base math utilities
// Located in: utils/MathCore.mt

public class MathCore {
    public function square(int x) : int {
        return x * x;
    }

    public function cube(int x) : int {
        return x * x * x;
    }
}

public function abs(int x) : int {
    if (x < 0) {
        return -x;
    }
    return x;
}

public function max(int a, int b) : int {
    if (a > b) {
        return a;
    }
    return b;
}

private function internalMathHelper(int x) : int {
    return x + 100;
}

public int MATH_VERSION = 1;
