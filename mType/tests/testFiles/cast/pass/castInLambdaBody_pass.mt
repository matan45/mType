// Test: Casting in lambda body
class Processor {
    public int value;

    public Processor(int v) {
        this.value = v;
    }
}

class AdvancedProcessor extends Processor {
    public int multiplier;

    public AdvancedProcessor(int v, int m) {
        super(v);
        this.multiplier = m;
    }

    public function process(): int {
        return this.value * this.multiplier;
    }
}

// Interface for lambda
interface Transformer<T, R> {
    public function apply(T input): R;
}

// Lambda with cast in body
Processor p = new AdvancedProcessor(10, 3);
Transformer<Processor, int> transformer = (Processor proc) -> {
    return ((AdvancedProcessor)proc).process();
};

int result = transformer.apply(p);
print(result);

// Lambda with multiple casts
interface Comparator<T> {
    public function compare(T a, T b): int;
}

Comparator<Processor> comp = (Processor a, Processor b) -> {
    int aVal = ((AdvancedProcessor)a).process();
    int bVal = ((AdvancedProcessor)b).process();
    if (aVal > bVal) {
        return 1;
    } else if (aVal < bVal) {
        return -1;
    } else {
        return 0;
    }
};

Processor p1 = new AdvancedProcessor(5, 2);
Processor p2 = new AdvancedProcessor(7, 2);
int comparison = comp.compare(p1, p2);
if (comparison < 0) {
    print("p1 < p2");
} else if (comparison > 0) {
    print("p1 > p2");
} else {
    print("p1 == p2");
}

// Expression lambda with cast
interface Extractor<T> {
    public function extract(T input): int;
}

Extractor<Processor> extractor = (Processor proc) -> ((AdvancedProcessor)proc).multiplier;
int multiplier = extractor.extract(p);
print("Multiplier: " + multiplier);

// Lambda with cast and conditional
interface Validator<T> {
    public function isValid(T input): bool;
}

Validator<Processor> validator = (Processor proc) -> {
    if (proc isClassOf AdvancedProcessor) {
        return ((AdvancedProcessor)proc).multiplier > 0;
    } else {
        return false;
    }
};

if (validator.isValid(p1)) {
    print("Valid processor");
}

// Expected output:
// 30
// p1 < p2
// Multiplier: 3
// Valid processor
