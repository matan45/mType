// Multi-parameter lambda test
interface BinaryFunction {
    function apply(a: int, b: int) : int;
}

interface Comparator {
    function compare(a: int, b: int) : bool;
}

print("=== Multi-Parameter Lambda Test ===");

BinaryFunction adder = (a, b) -> a + b;
BinaryFunction multiplier = (x, y) -> x * y;

print("3 + 5 = " + adder.apply(3, 5));
print("4 * 6 = " + multiplier.apply(4, 6));

Comparator isGreater = (a, b) -> a > b;
print("Is 10 > 5? " + isGreater.compare(10, 5));
print("Is 3 > 8? " + isGreater.compare(3, 8));