// Predicate interface lambda test
interface Predicate {
    function test(x: int) : bool;
}

print("=== Predicate Interface Test ===");

Predicate isEven = x -> (x % 2) == 0;
Predicate isPositive = x -> x > 0;

print("Is 4 even? " + isEven.test(4));
print("Is 7 even? " + isEven.test(7));
print("Is 10 positive? " + isPositive.test(10));
print("Is -5 positive? " + isPositive.test(-5));