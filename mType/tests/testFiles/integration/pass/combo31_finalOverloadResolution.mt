// Combo 31: Final + Overload Resolution
// Tests: subclass overrides only the non-final overload; both still dispatch

class Calc {
    public final function compute(int a): int {
        return a * 10;
    }

    public function compute(int a, int b): int {
        return a + b;
    }

    public function name(): string {
        return "Calc";
    }
}

class FancyCalc extends Calc {
    // Cannot override compute(int) - it's final
    // Override compute(int, int) - non-final
    public function compute(int a, int b): int {
        return (a + b) * 100;
    }

    public function name(): string {
        return "FancyCalc";
    }
}

function main(): void {
    print("=== Combo 31: Final + Overload Resolution ===");

    print("--- Base Calc ---");
    Calc base = new Calc();
    print(base.name() + " compute(7) = " + base.compute(7));
    print(base.name() + " compute(3, 4) = " + base.compute(3, 4));

    print("--- FancyCalc ---");
    FancyCalc fancy = new FancyCalc();
    print(fancy.name() + " compute(7) = " + fancy.compute(7));
    print(fancy.name() + " compute(3, 4) = " + fancy.compute(3, 4));

    print("--- Polymorphic dispatch ---");
    Calc poly = new FancyCalc();
    print(poly.name() + " compute(2) = " + poly.compute(2));
    print(poly.name() + " compute(5, 6) = " + poly.compute(5, 6));

    print("=== Combo 31 Complete ===");
}

main();
