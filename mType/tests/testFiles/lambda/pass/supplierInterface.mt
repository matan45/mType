// Supplier interface lambda test
interface Supplier {
    function get() : int;
}

print("=== Supplier Interface Test ===");

Supplier numberSupplier = () -> 42;
Supplier randomSupplier = () -> 17;

print("First supplier: " + numberSupplier.get());
print("Second supplier: " + randomSupplier.get());