// Test: An overloaded static call returning a primitive flows through binary
// arithmetic. inferFunctionCallType must recover INT for both overloads so
// the surrounding ADD type-checks.

class Calc {
    public static function compute(int x): int {
        return x * 2;
    }
    public static function compute(int x, int y): int {
        return x + y;
    }
}

int total = Calc::compute(5) + Calc::compute(3, 4);
print("total=" + total);
