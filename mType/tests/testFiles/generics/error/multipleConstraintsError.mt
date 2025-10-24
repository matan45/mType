// Test multiple constraints error (intersection types)
interface Serializable {
    function serialize(): string;
}

interface Comparable<T> {
    function compareTo(T other): int;
}

class Data {
    int value;
}

// ERROR: Multiple interface constraints may not be supported
function <T extends Serializable & Comparable<T>> process(T item): void {
    print("Processing item");
}

function main(): void {
    // This should trigger an error
    print("This should not execute");
}

main();
