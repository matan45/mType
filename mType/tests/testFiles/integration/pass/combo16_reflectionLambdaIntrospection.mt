// Combo 16: Reflection over a class holding lambda fields (functional-interface typed)
// Tests: discover lambda-typed fields and reflect on the functional interface itself

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Method.mt";

interface IntMapper {
    function apply(int x): int;
}

interface IntCombiner {
    function combine(int a, int b): int;
}

class Calculator {
    public IntMapper doubler;
    public IntCombiner adder;
    private int seed;

    public constructor(IntMapper d, IntCombiner a, int seed) {
        this.doubler = d;
        this.adder = a;
        this.seed = seed;
    }

    public function run(int x, int y): int {
        return this.adder.combine(this.doubler.apply(x), this.doubler.apply(y)) + this.seed;
    }
}

function main(): void {
    print("=== Combo 16: Reflection + Lambda Introspection ===");

    IntMapper twice = x -> x * 2;
    IntCombiner sum = (a, b) -> a + b;

    Calculator calc = new Calculator(twice, sum, 1);
    print("--- Direct lambda calls ---");
    print("doubler(5) = " + calc.doubler.apply(5));
    print("adder(3, 4) = " + calc.adder.combine(3, 4));
    print("run(2, 3) = " + calc.run(2, 3));

    print("--- Calculator fields ---");
    Class calcCls = Class::forName("Calculator");
    print("Class: " + calcCls.getName());
    Field[] fields = calcCls.getDeclaredFields();
    print("Field count: " + fields.length);
    for (int i = 0; i < fields.length; i++) {
        Field f = fields[i];
        print("Field: " + f.getName() + " type=" + f.getType());
    }

    print("--- Calculator methods ---");
    Method[] methods = calcCls.getDeclaredMethods();
    print("Method count: " + methods.length);
    for (int i = 0; i < methods.length; i++) {
        print("Method: " + methods[i].getName());
    }

    print("=== Combo 16 Complete ===");
}

main();
