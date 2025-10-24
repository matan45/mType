// Test incomplete type arguments error
class Pair<K, V> {
    K key;
    V value;

    public function Pair(K k, V v) {
        key = k;
        value = v;
    }
}

function main(): void {
    // ERROR: Missing second type argument (V)
    Pair<int> pair = new Pair<int>();

    print("This should not execute");
}

main();
