// Map<K,V> - Key-value pairs using parallel arrays
class Map<K,V> {
    K[] keys;
    V[] values;
    int capacity;
    int count;
        constructor() {
            this.capacity = 4;
            this.keys = new K[this.capacity];
            this.values = new V[this.capacity];
            this.count = 0;
        }

        function put(K key, V value): void {
            int index = this.findKeyIndex(key);
            if (index >= 0) {
                // Update existing
                this.values[index] = value;
            } else {
                // Add new
                if (this.count >= this.capacity) {
                    this.resize();
                }
                this.keys[this.count] = key;
                this.values[this.count] = value;
                this.count++;
            }
        }

        function get(K key): V {
            int index = this.findKeyIndex(key);
            if (index >= 0) {
                return this.values[index];
            }
            return null; // Key not found
        }

        function containsKey(K key): bool {
            return this.findKeyIndex(key) >= 0;
        }

        function remove(K key): bool {
            int index = this.findKeyIndex(key);
            if (index >= 0) {
                // Shift elements left
                for (int i = index; i < this.count - 1; i++) {
                    this.keys[i] = this.keys[i + 1];
                    this.values[i] = this.values[i + 1];
                }
                this.count--;
                return true;
            }
            return false;
        }

        function size(): int {
            return this.count;
        }

        function empty(): bool {
            return this.count == 0;
        }

        function clear(): void {
            this.count = 0;
        }

        // Get all keys
        function getKeys(): K[] {
            K[] result = new K[this.count];
            int index = 0;
            K[] currentKeys = this.getKeysInternal();
            for (K key : currentKeys) {
                result[index] = key;
                index++;
            }
            return result;
        }

        // Internal helper to avoid recursion
        function getKeysInternal(): K[] {
            K[] result = new K[this.count];
            for (int i = 0; i < this.count; i++) {
                result[i] = this.keys[i];
            }
            return result;
        }

        // Get all values
        function getValues(): V[] {
            V[] result = new V[this.count];
            int index = 0;
            V[] currentValues = this.getValuesInternal();
            for (V value : currentValues) {
                result[index] = value;
                index++;
            }
            return result;
        }

        // Internal helper to avoid recursion
        function getValuesInternal(): V[] {
            V[] result = new V[this.count];
            for (int i = 0; i < this.count; i++) {
                result[i] = this.values[i];
            }
            return result;
        }

        function findKeyIndex(K key): int {
            for (int i = 0; i < this.count; i++) {
                if (this.keys[i] == key) {
                    return i;
                }
            }
            return -1;
        }

        function resize(): void {
            this.capacity = this.capacity * 2;
            K[] newKeys = new K[this.capacity];
            V[] newValues = new V[this.capacity];
            for (int i = 0; i < this.count; i++) {
                newKeys[i] = this.keys[i];
                newValues[i] = this.values[i];
            }
            this.keys = newKeys;
            this.values = newValues;
        }
}