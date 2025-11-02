// Test lambda parameter type inference
interface Transformer {
    function transform(int value) : int;
}

interface StringProcessor {
    function process(string text) : string;
}

print("=== Lambda Parameter Type Inference Test ===");

// Infer int parameter type from interface
Transformer doubler = x -> x * 2;
int result1 = doubler.transform(10);
print("Doubled 10: " + result1);

// Infer string parameter type from interface
StringProcessor upper = s -> s + "!";
string result2 = upper.process("Hello");
print("Processed: " + result2);

// Multi-parameter inference
interface BinaryOp {
    function operate(int a, int b) : int;
}

BinaryOp adder = (x, y) -> x + y;
int result3 = adder.operate(5, 7);
print("5 + 7 = " + result3);

print("Parameter inference successful");
