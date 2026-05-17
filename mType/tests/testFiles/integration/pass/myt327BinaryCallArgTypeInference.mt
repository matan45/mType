// Regression for MYT-327: binary expressions made from call results must infer
// their primitive type when used directly as method-call arguments.

class IntOps {
    public static function a(): int { return 8; }
    public static function b(): int { return 3; }
}

class BoolOps {
    public static function t(): bool { return true; }
    public static function f(): bool { return false; }
}

class Sink {
    public function takeInt(int x, int y, int z): int {
        return x + y + z;
    }

    public static function takeInt(int x, int y, int z): int {
        return x + y + z;
    }

    public function takeBool(int x, bool y, int z): int {
        if (y) {
            return x + z + 1;
        }
        return x + z;
    }

    public static function takeBool(int x, bool y, int z): int {
        if (y) {
            return x + z + 1;
        }
        return x + z;
    }

    public function takeBoolLast(int x, int y, bool z): int {
        if (z) {
            return x + y + 1;
        }
        return x + y;
    }

    public static function takeBoolLast(int x, int y, bool z): int {
        if (z) {
            return x + y + 1;
        }
        return x + y;
    }
}

function main(): void {
    Sink s = new Sink();
    int total = 0;

    total = total + s.takeInt(1, IntOps::a() | IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() | IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() | IntOps::b(), 2);
    int hoistedOr = IntOps::a() | IntOps::b();
    total = total + s.takeInt(1, hoistedOr, 2);

    total = total + s.takeInt(1, IntOps::a() & IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() & IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() & IntOps::b(), 2);
    int hoistedAnd = IntOps::a() & IntOps::b();
    total = total + s.takeInt(1, hoistedAnd, 2);

    total = total + s.takeInt(1, IntOps::a() ^ IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() ^ IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() ^ IntOps::b(), 2);
    int hoistedXor = IntOps::a() ^ IntOps::b();
    total = total + s.takeInt(1, hoistedXor, 2);

    total = total + s.takeInt(1, IntOps::a() << IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() << IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() << IntOps::b(), 2);
    int hoistedLeftShift = IntOps::a() << IntOps::b();
    total = total + s.takeInt(1, hoistedLeftShift, 2);

    total = total + s.takeInt(1, IntOps::a() >> IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() >> IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() >> IntOps::b(), 2);
    int hoistedRightShift = IntOps::a() >> IntOps::b();
    total = total + s.takeInt(1, hoistedRightShift, 2);

    total = total + s.takeInt(1, IntOps::a() + IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() + IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() + IntOps::b(), 2);
    int hoistedPlus = IntOps::a() + IntOps::b();
    total = total + s.takeInt(1, hoistedPlus, 2);

    total = total + s.takeInt(1, IntOps::a() - IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() - IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() - IntOps::b(), 2);
    int hoistedMinus = IntOps::a() - IntOps::b();
    total = total + s.takeInt(1, hoistedMinus, 2);

    total = total + s.takeInt(1, IntOps::a() * IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() * IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() * IntOps::b(), 2);
    int hoistedMultiply = IntOps::a() * IntOps::b();
    total = total + s.takeInt(1, hoistedMultiply, 2);

    total = total + s.takeInt(1, IntOps::a() / IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() / IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() / IntOps::b(), 2);
    int hoistedDivide = IntOps::a() / IntOps::b();
    total = total + s.takeInt(1, hoistedDivide, 2);

    total = total + s.takeInt(1, IntOps::a() % IntOps::b(), 2);
    total = total + s.takeInt(1, 2, IntOps::a() % IntOps::b());
    total = total + Sink::takeInt(1, IntOps::a() % IntOps::b(), 2);
    int hoistedModulo = IntOps::a() % IntOps::b();
    total = total + s.takeInt(1, hoistedModulo, 2);

    total = total + s.takeBool(1, BoolOps::t() || BoolOps::f(), 2);
    total = total + s.takeBoolLast(1, 2, BoolOps::t() || BoolOps::f());
    total = total + Sink::takeBool(1, BoolOps::t() || BoolOps::f(), 2);
    bool hoistedOrBool = BoolOps::t() || BoolOps::f();
    total = total + s.takeBool(1, hoistedOrBool, 2);

    total = total + s.takeBool(1, BoolOps::t() && BoolOps::f(), 2);
    total = total + s.takeBoolLast(1, 2, BoolOps::t() && BoolOps::f());
    total = total + Sink::takeBool(1, BoolOps::t() && BoolOps::f(), 2);
    bool hoistedAndBool = BoolOps::t() && BoolOps::f();
    total = total + s.takeBool(1, hoistedAndBool, 2);

    print("MYT-327 total = " + total);
}

main();
