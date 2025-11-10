// Lambda capturing another lambda's captures test
interface Function {
    function apply(int x) : int;
}

print("=== Double Closure Test ===");

int base = 10;

Function createMultiplier = mult -> {
    // This lambda captures 'base' and 'mult'
    Function multiplier = x -> {
        // This inner lambda captures everything from outer lambda
        return base + (mult * x);
    };
    return multiplier;
};

Function mult5 = createMultiplier.apply(5);
print("mult5(3) = " + mult5.apply(3));  // 10 + (5 * 3) = 25

Function mult10 = createMultiplier.apply(10);
print("mult10(3) = " + mult10.apply(3));  // 10 + (10 * 3) = 40

// Change base to see closure captured it
base = 100;
print("After changing base:");
print("mult5(3) = " + mult5.apply(3));  // 100 + (5 * 3) = 115

// Triple nesting
int a = 1;
Function triple = x -> {
    int b = 2;
    Function middle = y -> {
        int c = 3;
        Function inner = z -> {
            return a + b + c + x + y + z;
        };
        return inner.apply(y);
    };
    return middle.apply(x);
};

print("Triple nesting: " + triple.apply(10));  // 1+2+3+10+10+10 = 36

print("Double closure complete");
