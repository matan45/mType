// Basic expression lambda test
interface Function {
    function apply(int x) : int;
}

print("=== Basic Expression Lambda Test ===");

Function doubler = x -> x * 2;
print("5 doubled is: " + doubler.apply(5));

Function tripler = x -> x * 3;
print("4 tripled is: " + tripler.apply(4));