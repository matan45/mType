// Test recursive type bounds error
// <T extends Box<U>, U extends List<T>> creates circular dependency

class Box<T> {
    T value;
}

class List<T> {
    T item;
}

// ERROR: Circular type parameter dependency
function <T extends Box<U>, U extends List<T>> process(T first, U second): void {
    print("Processing");
}

function main(): void {
    // This should trigger an error
}

main();
