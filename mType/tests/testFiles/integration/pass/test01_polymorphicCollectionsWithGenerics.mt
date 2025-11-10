// Integration Test 01: Polymorphic Collections with Generics
// Tests: Interfaces + Generics + Collections + Lambdas + isClassOf

import * from "../../lib/collections/List.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic interface with type parameter
interface Processor<T> {
    function process(T item): String;
    function getPriority(): int;
}

// Concrete implementations
class IntProcessor implements Processor<Int> {
    private int multiplier;

    constructor(int mult) {
        this.multiplier = mult;
    }

    public function process(Int item): String {
        int result = item.getValue() * this.multiplier;
        return new String("IntProcessor: " + result);
    }

    public function getPriority(): int {
        return 1;
    }
}

class StringProcessor implements Processor<String> {
    private string prefix;

    constructor(string pre) {
        this.prefix = pre;
    }

    public function process(String item): String {
        return new String(this.prefix + item.getValue());
    }

    public function getPriority(): int {
        return 2;
    }
}

// Generic container class
class ProcessorContainer<T> {
    private Processor<T> processor;
    private List<T> items;

    constructor(Processor<T> proc) {
        this.processor = proc;
        this.items = new List<T>();
    }

    public function addItem(T item): void {
        this.items.add(item);
    }

    public function processAll(): void {
        for (int i = 0; i < this.items.size(); i = i + 1) {
            T item = this.items.get(i);
            String result = this.processor.process(item);
            print(result.getValue());
        }
    }

    public function getProcessor(): Processor<T> {
        return this.processor;
    }
}

// Test polymorphism with isClassOf
function testProcessorType(Processor<Int> proc): void {
    if (proc isClassOf IntProcessor) {
        print("Type check: IntProcessor detected");
    } else {
        print("Type check: Unknown processor");
    }
}

// Main test execution
print("=== Test 01: Polymorphic Collections with Generics ===");

// Test 1: IntProcessor with generic container
ProcessorContainer<Int> intContainer = new ProcessorContainer<Int>(new IntProcessor(10));
intContainer.addItem(new Int(5));
intContainer.addItem(new Int(7));
intContainer.addItem(new Int(3));

print("Processing integers:");
intContainer.processAll();

// Test 2: StringProcessor with generic container
ProcessorContainer<String> stringContainer = new ProcessorContainer<String>(new StringProcessor("PREFIX-"));
stringContainer.addItem(new String("Alpha"));
stringContainer.addItem(new String("Beta"));
stringContainer.addItem(new String("Gamma"));

print("Processing strings:");
stringContainer.processAll();

// Test 3: Type checking with isClassOf
Processor<Int> procRef = intContainer.getProcessor();
testProcessorType(procRef);

// Test 4: Lambda with generics (functional interface pattern)
interface Transformer<T, R> {
    function transform(T input): R;
}

class LambdaHolder<T, R> {
    private Transformer<T, R> transformer;

    public function setTransformer(Transformer<T, R> trans): void {
        this.transformer = trans;
    }

    public function apply(T input): R {
        return this.transformer.transform(input);
    }
}

// Create lambda-based transformer
class IntToStringTransformer implements Transformer<Int, String> {
    public function transform(Int input): String {
        return new String("Transformed: " + input.getValue());
    }
}

LambdaHolder<Int, String> holder = new LambdaHolder<Int, String>();
holder.setTransformer(new IntToStringTransformer());
String lambdaResult = holder.apply(new Int(42));
print(lambdaResult.getValue());

// Test 5: Priority comparison
print("Priority comparison:");
Processor<Int> p1 = new IntProcessor(5);
Processor<String> p2 = new StringProcessor("TEST-");

if (p1.getPriority() < p2.getPriority()) {
    print("IntProcessor has lower priority");
} else {
    print("StringProcessor has lower priority");
}

print("=== Test 01 Complete ===");
