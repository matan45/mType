// Test interface with complex generic parameters
interface TestInterface<T, U> {
    function complexMethod(Repository<T, U> param): Repository<T, U>;
    function simpleMethod(T simple): U;
}

interface Repository<K, V> {
    function get(K key): V;
    function put(K key, V value): void;
}

print("Complex generic parameter parsing test completed successfully!");