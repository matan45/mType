// Generic interface with constraints test
interface Comparable<T> {
    function compare(T other): int;
}

interface Container<T> {
    function add(T item): bool;
    function size(): int;
}

interface SortableContainer<T extends Comparable<T>> extends Container<T> {
    function sort(): void;
    function findMin(): T;
}

print("Generic constraints successful");
