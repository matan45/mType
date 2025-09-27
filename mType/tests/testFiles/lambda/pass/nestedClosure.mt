// Nested closure test
interface Function {
    function apply(int x) : int;
}

print("=== Nested Closure Test ===");

function createMultiplier(int factor) : Function {
    Function multiplier = x -> x * factor;
    return multiplier;
}

Function doubler = createMultiplier(2);
Function tripler = createMultiplier(3);

print("Double 5: " + doubler.apply(5));
print("Triple 4: " + tripler.apply(4));