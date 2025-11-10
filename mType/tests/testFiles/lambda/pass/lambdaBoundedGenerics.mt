// Lambda with bounded type parameters test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Comparable {
    function compareTo(Comparable other) : int;
}

interface Function<T, R> {
    function apply(T input) : R;
}

class Number implements Comparable {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function compareTo(Comparable other) : int {
        Number n = (Number)other;
        return this.value - n.value;
    }

    public function getValue() : int {
        return this.value;
    }
}

print("=== Bounded Generics Test ===");

// Lambda with bounded generic type
Function<Number, Int> extractor = n -> new Int(n.getValue());

Number n1 = new Number(42);
Number n2 = new Number(100);

print("Extract from n1: " + extractor.apply(n1).getValue());
print("Extract from n2: " + extractor.apply(n2).getValue());

// Lambda comparing bounded types
Function<Number, String> classifier = n -> {
    int val = n.getValue();
    return val > 50 ? new String("large") : new String("small");
};

print("n1 is: " + classifier.apply(n1).getValue());
print("n2 is: " + classifier.apply(n2).getValue());

print("Bounded generics complete");
