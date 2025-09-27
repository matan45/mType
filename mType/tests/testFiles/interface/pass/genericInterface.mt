// Generic interface test
interface Container<T> {
    function add(T item): bool;
    function get(int index): T;
    function size(): int;
}

interface Comparable<T> {
    function compare(T other): int;
}

print("Generic interface successful");