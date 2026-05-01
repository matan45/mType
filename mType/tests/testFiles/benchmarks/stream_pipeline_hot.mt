// TARGET: smoke/perf coverage for stream pipeline execution.
// Exercises stream filter/map/count pipeline execution. Keep this as one large
// stream pipeline and below the current JIT/OSR instability threshold.

import * from "../lib/collections/ArrayList.mt";
import * from "../lib/stream/Stream.mt";
import * from "../lib/stream/Streams.mt";
import * from "../lib/functional/Predicate.mt";
import * from "../lib/functional/Function.mt";
import * from "../lib/primitives/Int.mt";

ArrayList<Int> data = new ArrayList<Int>();
int M = 8;
for (int i = 0; i < M; i = i + 1) {
    data.add(new Int(i));
}

int N = 5000;
int total = 0;
for (int j = 0; j < N; j = j + 1) {
    Int count = data.stream()
        .filter(x-> x.getValue() % 2 == 0)
        .map(x-> new Int(x.getValue() * 3))
        .count();
    total = total + count.getValue();
}


print("stream_pipeline_hot total=" + total);

