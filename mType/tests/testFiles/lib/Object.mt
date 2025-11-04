interface Object<T> {
    function toString(): string;
    function equals(T other) : bool;
    function hashCode() : int;
}