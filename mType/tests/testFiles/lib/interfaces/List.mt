// List<T> - Ordered collection with positional access
// Allows duplicate elements and provides indexed operations
// Maintains insertion order

import * from "../Collection.mt";
import * from "../Iterator.mt";

interface List<T> extends Collection<T> {
    // ===== Positional access operations =====

    // Returns the element at the specified position
    function get(int index): T;

    // Replaces the element at the specified position
    function set(int index, T item): void;

    // Removes the element at the specified position
    // Returns true if an element was removed
    function removeAt(int index): bool;

    // ===== Search operations =====

    // Returns the index of the first occurrence of the specified element
    // Returns -1 if not found
    function indexOf(T item): int;

    // Returns the index of the last occurrence of the specified element
    // Returns -1 if not found
    function lastIndexOf(T item): int;

    // ===== Boundary operations =====

    // Returns the first element in the list
    function first(): T;

    // Returns the last element in the list
    function last(): T;

    // ===== List-specific operations =====

    // Reverses the order of elements in this list
    function reverse(): void;

    // Sorts this list into ascending order
    // Requires T implements Comparable<T>
    function sort(): void;

    // ===== Inherited from Collection<T> =====
    // function add(T item): bool;
    // function remove(T item): bool;
    // function contains(T item): bool;
    // function size(): int;
    // function empty(): bool;
    // function clear(): void;
    // function addAll(T[] items): void;
    // function hashCode(): int;
    // function iterator(): Iterator<T>;
    // function toArray(): T[];
}
