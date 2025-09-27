// Import base interfaces
import "BaseInterfaces.mt";

// Single inheritance with multiple generic types
interface SortableMap<K, V> extends Comparable<K> {
    function put(K key, V value): void;
    function get(K key): V;
    function size(): int;
}

// Repository interface with two generic types
interface Repository<T, ID> extends Hashable {
    function save(T entity): T;
    function findById(ID id): T;
    function delete(ID id): void;
    function equals(Repository<T, ID> other): bool;
}

// Cache interface with three generic types
interface CacheStore<K, V, M> extends Repository<V, K> {
    function cache(K key, V value, M metadata): void;
    function evict(K key): bool;
    function getMetadata(K key): M;
}

// Functional interface with multiple generic types
interface BiFunction<T, U, R> {
    function apply(T first, U second): R;
}

// Predicate interface with two generics
interface BiPredicate<T, U> {
    function test(T first, U second): bool;
}