// Isolates String boxed-method dispatch (INVOKE_STRING_* + StringPool intern).
// Companion to boxed_primitive_dispatch_hot.mt which mixes Int/Float/Bool/String;
// this one stays pure-String so a regression in the String fast path or the
// pool-intern hit rate shows up clearly.
//
// TARGET: ~1-2s on dev machine. Adjust N if first run lands outside 1-5s.
//
// Hot loop calls per iteration: 1× concat, 1× length, 2× equals, 1× isEmpty.
// concat result cycles through 11 unique strings, so after warmup every
// concat hits the StringPool instead of allocating a new bridge.

import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/String.mt";

int N = 500000;
String prefix = new String("row-");
String target = new String("row-7");
String empty = new String("");

int matches = 0;
int totalLen = 0;
int emptyHits = 0;

for (int i = 0; i < N; i = i + 1) {
    String suffix = new String("" + (i % 11));
    String combined = prefix.concat(suffix);
    totalLen = totalLen + combined.length();
    if (combined.equals(target)) {
        matches = matches + 1;
    }
    if (empty.isEmpty()) {
        emptyHits = emptyHits + 1;
    }
    if (suffix.equals(empty)) {
        emptyHits = emptyHits + 1;
    }
}

print("boxed_string_dispatch_hot len=" + totalLen + " matches=" + matches + " empty=" + emptyHits);
