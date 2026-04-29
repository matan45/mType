// Combo 35: Lambda stored in a static (final) field, applied via static accessor
// Tests: Function<Int,Int> declared at class scope, retrieved through a static method,
// then applied to several boxed inputs

import * from "../../lib/functional/Function.mt";
import * from "../../lib/primitives/Int.mt";

class MathOps {
    public static final Function<Int, Int> DOUBLER = x -> new Int(x.getValue() * 2);
    public static final Function<Int, Int> SQUARE = x -> new Int(x.getValue() * x.getValue());

    public static function getDoubler(): Function<Int, Int> {
        return DOUBLER;
    }

    public static function getSquare(): Function<Int, Int> {
        return SQUARE;
    }
}

function main(): void {
    print("=== Combo 35: Lambda in Static Initializer ===");

    Function<Int, Int> dbl = MathOps::getDoubler();
    Function<Int, Int> sq = MathOps::getSquare();

    Int[] inputs = [new Int(1), new Int(2), new Int(3), new Int(4), new Int(5)];

    print("--- DOUBLER ---");
    for (int i = 0; i < inputs.length; i++) {
        Int r = dbl.apply(inputs[i]);
        print(inputs[i].getValue() + " -> " + r.getValue());
    }

    print("--- SQUARE ---");
    for (int i = 0; i < inputs.length; i++) {
        Int r = sq.apply(inputs[i]);
        print(inputs[i].getValue() + " -> " + r.getValue());
    }

    print("--- Direct field access ---");
    Int seven = new Int(7);
    print("doubler(7) = " + MathOps::DOUBLER.apply(seven).getValue());
    print("square(7) = " + MathOps::SQUARE.apply(seven).getValue());

    print("=== Combo 35 Complete ===");
}

main();
