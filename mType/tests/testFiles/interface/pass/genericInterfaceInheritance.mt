// Generic interface inheritance test
interface Container<T> {
    function add(T item): bool;
    function size(): int;
}

interface SortableContainer<T> extends Container<T> {
    function sort(): void;
    function get(int index): T;
}

print("Generic interface inheritance successful");