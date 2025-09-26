// Closure with block lambda test
interface Processor {
    function process(n: int) : int;
}

print("=== Closure Block Lambda Test ===");

function createAccumulator(initial: int) : Processor {
    Processor accumulator = n -> {
        int result = initial + n;
        if (result > 100) {
            return 100;
        }
        return result;
    };
    return accumulator;
}

Processor acc1 = createAccumulator(50);
Processor acc2 = createAccumulator(10);

print("Acc1 with 30: " + acc1.process(30));
print("Acc1 with 60: " + acc1.process(60));
print("Acc2 with 25: " + acc2.process(25));