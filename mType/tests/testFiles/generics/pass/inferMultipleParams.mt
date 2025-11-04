import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Multiple type parameter inference
class Triple<A, B, C> {
    A first;
    B second;
    C third;

    public function set(A a, B b, C c): void {
        first = a;
        second = b;
        third = c;
    }

    public function print(): void {
        print("First: " + first);
        print("Second: " + second);
        print("Third: " + third);
    }
}

function <A, B, C> makeTriple(A a, B b, C c): Triple<A, B, C> {
    Triple<A, B, C> t = new Triple<A, B, C>();
    t.set(a, b, c);
    return t;
}

function main(): void {
    // Infer A=Int, B=String, C=Int from arguments
    Triple<Int, String, Int> triple = makeTriple<Int, String, Int>(
        new Int(1),
        new String("middle"),
        new Int(3)
    );
    triple.print();
}

main();
