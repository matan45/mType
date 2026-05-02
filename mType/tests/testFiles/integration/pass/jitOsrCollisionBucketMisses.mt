// MYT-259 regression: OSR-compiled HashMap.findKeyInBucket misses keys at
// index >= 1 in collision buckets when the inner for-loop's two JUMP_BACK
// back-edges (incr->header AND body->incr) both cross the OSR threshold.
// Pre-fix observed: hits=1872 gotNonNull=1871. Post-fix expected: 2000/2000.
// The combination of containsKey + get is required: each calls
// findKeyInBucket, doubling the back-edge fires per outer iteration so the
// inner-loop OSR threshold (default 500) trips on collision-bucket scans.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

int M = 16;
HashMap<Int, String> map = new HashMap<Int, String>();
for (int i = 0; i < M; i = i + 1) {
    map.put(new Int(i), new String("v" + i));
}

int hits = 0;
int gotNonNull = 0;
for (int j = 0; j < 2000; j = j + 1) {
    Int key = new Int(j % M);
    if (map.containsKey(key)) {
        hits = hits + 1;
    }
    String v = map.get(key);
    if (v != null) {
        gotNonNull = gotNonNull + 1;
    }
}
print("hits=" + hits + " gotNonNull=" + gotNonNull);
