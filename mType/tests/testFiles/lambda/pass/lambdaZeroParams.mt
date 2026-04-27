// Zero-parameter lambda implementing Supplier-style interface
interface Supplier {
    function get(): int;
}

print("=== Lambda Zero Params Test ===");

Supplier answer = () -> 42;

print("first call: " + answer.get());
print("second call: " + answer.get());
