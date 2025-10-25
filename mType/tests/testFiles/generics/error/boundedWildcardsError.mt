// Test bounded wildcards error (if not supported)
import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";

class Number {
    int value;
}

class Integer extends Number {
    public function Integer(int val) {
        value = val;
    }
}

function main(): void {
    // ERROR: Bounded wildcards like ? extends Number may not be supported
    List<? extends Number> numbers = new List<Integer>();

    print("This should not execute");
}

main();
