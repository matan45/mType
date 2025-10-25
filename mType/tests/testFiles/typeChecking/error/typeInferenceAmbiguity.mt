// ERROR: Type inference ambiguity

class Box<T> {
    T value;
}

// Generic function with ambiguous type inference
function <T> createPair(T first, T second): Box<T> {
    Box<T> box = new Box<T>();
    // Ambiguous: should T be int or string?
    return box;
}

function main(): void {
    // ERROR: Cannot infer T when arguments have different types
    Box<int> result = createPair<int>(42, "string");

    print("This should not execute");
}

main();
