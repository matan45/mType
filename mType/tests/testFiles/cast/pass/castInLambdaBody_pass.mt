// Test: Casting in lambda body
import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";

class Processor {
    public int value;

    constructor(int v) {
        this.value = v;
    }
}

class AdvancedProcessor extends Processor {
    public int multiplier;

    constructor(int v, int m):super(v) {
        this.multiplier = m;
    }

    public function process(): int {
        return this.value * this.multiplier;
    }
}

// Interface for lambda - using Int wrapper for generic compatibility
interface Transformer<T, R> {
    public function apply(T input): R;
}

// Lambda with cast in body
Processor p = new AdvancedProcessor(10, 3);
Transformer<Processor, Int> transformer = proc -> {
    int result = ((AdvancedProcessor)proc).process();
    return new Int(result);
};

Int resultWrapper = transformer.apply(p);
print(resultWrapper.getValue());

// Lambda with multiple casts - using Int wrapper for return type
interface Comparator<T> {
    public function compare(T a, T b): Int;
}

Comparator<Processor> comp = (a, b) -> {
    int aVal = ((AdvancedProcessor)a).process();
    int bVal = ((AdvancedProcessor)b).process();
    int result;
    if (aVal > bVal) {
        result = 1;
    } else if (aVal < bVal) {
        result = -1;
    } else {
        result = 0;
    }
    return new Int(result);
};

Processor p1 = new AdvancedProcessor(5, 2);
Processor p2 = new AdvancedProcessor(7, 2);
Int comparisonWrapper = comp.compare(p1, p2);
int comparison = comparisonWrapper.getValue();
if (comparison < 0) {
    print("p1 < p2");
} else if (comparison > 0) {
    print("p1 > p2");
} else {
    print("p1 == p2");
}

// Expression lambda with cast - using Int wrapper
interface Extractor<T> {
    public function extract(T input): Int;
}

Extractor<Processor> extractor = proc -> {
    int mult = ((AdvancedProcessor)proc).multiplier;
    return new Int(mult);
};
Int multiplierWrapper = extractor.extract(p);
print("Multiplier: " + multiplierWrapper.getValue());

// Lambda with cast and conditional - using Bool wrapper
interface Validator<T> {
    public function isValid(T input): Bool;
}

Validator<Processor> validator = proc -> {
    bool isValid;
    if (proc isClassOf AdvancedProcessor) {
        isValid = ((AdvancedProcessor)proc).multiplier > 0;
    } else {
        isValid = false;
    }
    return new Bool(isValid);
};

Bool isValidWrapper = validator.isValid(p1);
if (isValidWrapper.getValue()) {
    print("Valid processor");
}

// Expected output:
// 30
// p1 < p2
// Multiplier: 3
// Valid processor
