// Base interfaces for inheritance testing
interface Comparable<T> {
    function compareTo(T other): int;
}

interface Hashable {
    function hashCode(): int;
}

interface Equatable<T> {
    function equals(T other): bool;
}

interface Serializable<T, U> {
    function serialize(): T;
    function deserialize(T data): U;
}