// Lambda with bounded type parameters test
interface Comparable {
    function compareTo(Comparable other) : int;
}

interface Function<T, R> {
    function apply(T input) : R;
}

class Number implements Comparable {
    int value;

    function init(int v) {
        this.value = v;
    }

    function compareTo(Comparable other) : int {
        Number n = (Number)other;
        return this.value - n.value;
    }

    function getValue() : int {
        return this.value;
    }
}

print("=== Bounded Generics Test ===");

// Lambda with bounded generic type
Function<Number, int> extractor = n -> n.getValue();

Number n1 = new Number(42);
Number n2 = new Number(100);

print("Extract from n1: " + extractor.apply(n1));
print("Extract from n2: " + extractor.apply(n2));

// Lambda comparing bounded types
Function<Number, String> classifier = n -> {
    int val = n.getValue();
    return val > 50 ? "large" : "small";
};

print("n1 is: " + classifier.apply(n1));
print("n2 is: " + classifier.apply(n2));

print("Bounded generics complete");
