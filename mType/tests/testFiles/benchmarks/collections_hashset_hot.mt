// HashSet-only hot path: stresses add/contains/remove on boxed primitive keys.
// Mirrors collections_hash_hot.mt but isolates HashSet from HashMap so we
// can measure each independently.

import * from "../lib/collections/HashSet.mt";
import * from "../lib/primitives/Int.mt";

int M = 256;
HashSet<Int> set = new HashSet<Int>();

for (int i = 0; i < M; i = i + 1) {
    set.add(new Int(i));
}

int N = 500000;
int hits = 0;

for (int j = 0; j < N; j = j + 1) {
    Int key = new Int(j % M);
    if (set.contains(key)) {
        hits = hits + 1;
    }
}

print("collections_hashset_hot hits=" + hits);
