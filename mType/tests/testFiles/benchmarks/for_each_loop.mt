// TARGET: ~2s on dev machine. Adjust N / M if first run lands outside 1-5s.
// Exercises GET_ITERATOR / ITERATOR_HAS_NEXT / ITERATOR_NEXT / ITERATOR_CLOSE
// via for-each over an ArrayList<Int>. The iteration runs inside a hot user
// function so the JIT function-level compiler picks it up once it crosses
// the hot threshold (workaround for MYT-148: OSR currently fails on
// top-level loops).
// Baseline coverage for MYT-147.

import * from "../lib/collections/ArrayList.mt";
import * from "../lib/primitives/Int.mt";

function sumForEach(ArrayList<Int> list): int {
    int s = 0;
    for (Int x : list) {
        s = s + x.getValue();
    }
    return s;
}

int M = 1000;
ArrayList<Int> data = new ArrayList<Int>();
for (int i = 0; i < M; i = i + 1) {
    data.add(new Int(i));
}

int N = 2000;
int total = 0;
for (int j = 0; j < N; j = j + 1) {
    total = total + sumForEach(data);
}

print("for_each_loop total=" + total);
