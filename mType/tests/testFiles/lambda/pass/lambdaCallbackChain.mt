// Multiple lambda callbacks in sequence test
interface Function<T, R> {
    function apply(T input) : R;
}

interface VoidFunction<T> {
    function execute(T input) : void;
}

class CallbackChain {
    VoidFunction<String>[] callbacks;
    int count;

    function init() {
        this.callbacks = new VoidFunction<String>[10];
        this.count = 0;
    }

    function addCallback(VoidFunction<String> callback) {
        this.callbacks[this.count] = callback;
        this.count = this.count + 1;
    }

    function execute(String input) {
        for (int i = 0; i < this.count; i = i + 1) {
            this.callbacks[i].execute(input);
        }
    }
}

class ValueTransformer<T> {
    Function<T, T>[] transformers;
    int count;

    function init() {
        this.transformers = new Function<T, T>[10];
        this.count = 0;
    }

    function addTransformer(Function<T, T> transformer) {
        this.transformers[this.count] = transformer;
        this.count = this.count + 1;
    }

    function transform(T value) : T {
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
ValueTransformer<int> transformer = new ValueTransformer<int>();

transformer.addTransformer(x -> x + 10);
transformer.addTransformer(x -> x * 2);
transformer.addTransformer(x -> x - 5);

int result1 = transformer.transform(5);  // ((5+10)*2)-5 = 25
print("Transform 5: " + result1);

int result2 = transformer.transform(10);  // ((10+10)*2)-5 = 35
print("Transform 10: " + result2);

// Conditional callback chain
VoidFunction<int>[] conditionalCallbacks = new VoidFunction<int>[3];
conditionalCallbacks[0] = x -> {
    if (x > 0) {
        print("Positive: " + x);
    }
};
conditionalCallbacks[1] = x -> {
    if (x % 2 == 0) {
        print("Even: " + x);
    }
};
conditionalCallbacks[2] = x -> {
    if (x > 10) {
        print("Large: " + x);
    }
};

print("Testing 15:");
for (int i = 0; i < 3; i = i + 1) {
    conditionalCallbacks[i].execute(15);
}

print("Testing -4:");
for (int i = 0; i < 3; i = i + 1) {
    conditionalCallbacks[i].execute(-4);
}

print("Callback chain complete");
