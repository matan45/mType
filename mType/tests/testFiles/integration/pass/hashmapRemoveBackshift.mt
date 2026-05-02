// Regression test for the linear-probing back-shift remove() correctness.
//
// The buggy version conflated the hole pointer with the probe pointer: when
// it encountered a "settled" entry (one whose ideal slot is between the hole
// and the probe), it advanced the hole instead of skipping past the entry.
// Result: the original deleted slot stayed occupied, OR a settled entry was
// overwritten by the next shifted entry. Either way, some keys disappeared
// from the map after a remove that had no business affecting them.
//
// This test uses many puts + a striped remove pattern so that across the
// resulting probe chains, the buggy pattern is virtually certain to occur
// at least once. Correct: every surviving key is still findable with its
// original value, and every removed key is gone.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/Int.mt";

// Force a tight starting capacity so resizes happen during fill and we get
// realistic collision density.
HashMap<Int, Int> map = new HashMap<Int, Int>(8);
HashSet<Int> set = new HashSet<Int>(8);

int N = 100;

for (int i = 0; i < N; i = i + 1) {
    map.put(new Int(i), new Int(i * 7));
    set.add(new Int(i));
}

// Remove every even key.
for (int i = 0; i < N; i = i + 2) {
    map.remove(new Int(i));
    set.remove(new Int(i));
}

// Every odd key must still be findable with its original value.
int mapHits = 0;
int mapValueOk = 0;
int setHits = 0;
for (int i = 1; i < N; i = i + 2) {
    Int v = map.get(new Int(i));
    if (v != null) {
        mapHits = mapHits + 1;
        if (v.getValue() == i * 7) {
            mapValueOk = mapValueOk + 1;
        }
    }
    if (set.contains(new Int(i))) {
        setHits = setHits + 1;
    }
}

// Every even key must be gone.
int mapStale = 0;
int setStale = 0;
for (int i = 0; i < N; i = i + 2) {
    if (map.containsKey(new Int(i))) {
        mapStale = mapStale + 1;
    }
    if (set.contains(new Int(i))) {
        setStale = setStale + 1;
    }
}

print("map.size=" + map.size());
print("set.size=" + set.size());
print("mapHits=" + mapHits);
print("mapValueOk=" + mapValueOk);
print("setHits=" + setHits);
print("mapStale=" + mapStale);
print("setStale=" + setStale);
print("Test passed");
