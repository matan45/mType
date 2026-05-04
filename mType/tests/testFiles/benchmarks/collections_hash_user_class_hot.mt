// MYT-274: user-class-keyed HashMap hot path. Mirrors collections_hash_hot.mt
// but with a 2-int-field user class (Point) as the key. With the synthesized
// fast hashCode/equals, runtime should land within ~2x of the Int-keyed
// benchmark (vs ~10-50x slower without synthesis, when the slow string-based
// Object default dominates).

import * from "../lib/collections/HashMap.mt";
import * from "../lib/collections/HashSet.mt";
import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/String.mt";

class Point {
    private int x;
    private int y;
    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

int M = 256;
HashMap<Point, String> map = new HashMap<Point, String>();
HashSet<Point> set = new HashSet<Point>();

for (int i = 0; i < M; i = i + 1) {
    Point key = new Point(i, i + 1);
    map.put(key, new String("v" + i));
    set.add(key);
}

int N = 500000;
int hits = 0;
int lenTotal = 0;

for (int j = 0; j < N; j = j + 1) {
    int v = j % M;
    Point key = new Point(v, v + 1);
    if (map.containsKey(key)) {
        hits = hits + 1;
        lenTotal = lenTotal + map.get(key).length();
    }
    if (set.contains(key)) {
        hits = hits + 1;
    }
}

print("collections_hash_user_class_hot hits=" + hits + " len=" + lenTotal);
