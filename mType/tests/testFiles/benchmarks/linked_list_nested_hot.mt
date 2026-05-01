// TARGET: smoke/perf coverage for LinkedList traversal under JIT/OSR.
// Exercises nested loops over a doubly-linked list via index-based
// access. Per outer iteration:
//   - inner walks all M elements via get(k) — each get() walks the
//     linked-list head→tail chasing Node.next pointers (so the inner
//     loop is O(M^2) overall, dense in field-access ops)
//   - reads element value via getValue() (boxed Int → primitive int)
//   - branches on parity, conditionally accumulates
// Mirrors stream_pipeline_hot's shape but on the linked-list backing
// rather than ArrayList, so the JIT touches Node<T>.next field chains
// and exercises method-call inlining at depth 2+ on a linked container.

import * from "../lib/collections/LinkedList.mt";
import * from "../lib/primitives/Int.mt";

LinkedList<Int> data = new LinkedList<Int>();
int M = 16;
for (int i = 0; i < M; i = i + 1) {
    data.add(new Int(i));
}

int N = 5000;
int total = 0;
int matches = 0;
for (int j = 0; j < N; j = j + 1) {
    int sum = 0;
    int count = 0;
    int k = 0;
    while (k < M) {
        Int next = data.get(k);
        int v = next.getValue();
        if (v % 2 == 0) {
            sum = sum + (v * 3);
            count = count + 1;
        }
        k = k + 1;
    }
    total = total + sum;
    matches = matches + count;
}

print("linked_list_nested_hot total=" + total + " matches=" + matches);
