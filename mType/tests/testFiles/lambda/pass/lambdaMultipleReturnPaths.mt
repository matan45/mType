// Multiple return statements test
interface Function {
    function apply(int x) : int;
}

print("=== Multiple Return Paths Test ===");

// Simple if-else returns
Function absolute = x -> {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
};

print("abs(-5) = " + absolute.apply(-5));
print("abs(10) = " + absolute.apply(10));
print("abs(0) = " + absolute.apply(0));

// Multiple conditions
Function classifier = x -> {
    if (x < 0) {
        return -1;
    } else if (x > 0) {
        return 1;
    } else {
        return 0;
    }
};

print("classify(-10) = " + classifier.apply(-10));
print("classify(0) = " + classifier.apply(0));
print("classify(42) = " + classifier.apply(42));

// Early returns
Function complexLogic = x -> {
    if (x == 0) {
        return 0;
    }
    if (x < 0) {
        return -1;
    }
    if (x < 10) {
        return 1;
    }
    if (x < 100) {
        return 2;
    }
    return 3;
};

print("complex(0) = " + complexLogic.apply(0));
print("complex(-5) = " + complexLogic.apply(-5));
print("complex(5) = " + complexLogic.apply(5));
print("complex(50) = " + complexLogic.apply(50));
print("complex(200) = " + complexLogic.apply(200));

print("Multiple return paths complete");
