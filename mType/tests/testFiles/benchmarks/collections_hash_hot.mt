// TARGET: ~1-3s on dev machine. Adjust N/M if first run lands outside that range.
// Exercises HashMap.get/containsKey and HashSet.contains on boxed primitive keys.

import * from "../lib/collections/HashMap.mt";
import * from "../lib/collections/HashSet.mt";
import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/String.mt";

int M = 256;
HashMap<Int, String> map = new HashMap<Int, String>();
HashSet<Int> set = new HashSet<Int>();

for (int i = 0; i < M; i = i + 1) {
    Int key = new Int(i);
    map.put(key, new String("v" + i));
    set.add(key);
}

int N = 500000;
int hits = 0;
int lenTotal = 0;

for (int j = 0; j < N; j = j + 1) {
    Int key = new Int(j % M);
    if (map.containsKey(key)) {
        hits = hits + 1;
        lenTotal = lenTotal + map.get(key).length();
    }
    if (set.contains(key)) {
        hits = hits + 1;
    }
}

print("collections_hash_hot hits=" + hits + " len=" + lenTotal);
