// Multiple lambda callbacks in sequence test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

interface VoidFunction<T> {
    function execute(T input) : void;
}

class CallbackChain {
    VoidFunction<String>[] callbacks;
    int count;

    constructor() {
        this.callbacks = new VoidFunction<String>[10];
        this.count = 0;
    }

    public function addCallback(VoidFunction<String> callback) {
        this.callbacks[this.count] = callback;
        this.count = this.count + 1;
    }

    public function execute(String input) {
        for (int i = 0; i < this.count; i = i + 1) {
            this.callbacks[i].execute(input);
        }
    }
}

class ValueTransformer<T> {
    Function<T, T>[] transformers;
    int count;

    constructor() {
        this.transformers = new Function<T, T>[10];
        this.count = 0;
    }

    public function addTransformer(Function<T, T> transformer) {
        this.transformers[this.count] = transformer;
        this.count = this.count + 1;
    }

    public function transform(T value) : T {
        T result = value;
        for (int i = 0; i < this.count; i = i + 1) {
            result = this.transformers[i].apply(result);
        }
        return result;
    }
}

print("=== Callback Chain Test ===");

CallbackChain chain = new CallbackChain();

chain.addCallback(msg -> print("Callback 1: " + msg));
chain.addCallback(msg -> print("Callback 2: " + msg + "!"));
chain.addCallback(msg -> print("Callback 3: " + msg + "?"));

chain.execute("Hello");

// Value transformation chain
ValueTransformer<Int> transformer = new ValueTransformer<Int>();

transformer.addTransformer(x -> new Int(x.getValue() + 10));
transformer.addTransformer(x -> new Int(x.getValue() * 2));
transformer.addTransformer(x -> new Int(x.getValue() - 5));

Int result1 = transformer.transform(new Int(5));  // ((5+10)*2)-5 = 25
print("Transform 5: " + result1.getValue());

Int result2 = transformer.transform(new Int(10));  // ((10+10)*2)-5 = 35
print("Transform 10: " + result2.getValue());

// Conditional callback chain
VoidFunction<Int>[] conditionalCallbacks = new VoidFunction<Int>[3];
conditionalCallbacks[0] = x -> {
    if (x.getValue() > 0) {
        print("Positive: " + x.getValue());
    }
};
conditionalCallbacks[1] = x -> {
    if (x.getValue() % 2 == 0) {
        print("Even: " + x.getValue());
    }
};
conditionalCallbacks[2] = x -> {
    if (x.getValue() > 10) {
        print("Large: " + x.getValue());
    }
};

print("Testing 15:");
for (int i = 0; i < 3; i = i + 1) {
    conditionalCallbacks[i].execute(new Int(15));
}

print("Testing -4:");
for (int i = 0; i < 3; i = i + 1) {
    conditionalCallbacks[i].execute(new Int(-4));
}

print("Callback chain complete");
