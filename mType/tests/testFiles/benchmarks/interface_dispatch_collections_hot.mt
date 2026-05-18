// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises interface-typed method dispatch through an ArrayList<Handler>
// iterator. Six implementations populate the call site at h.handle(x) past the
// POLY-4 width, so without a wider mega tier this is the runtime IC slow path
// — the iterator boundary also keeps the call site outside of inline candidacy.

import * from "../lib/collections/ArrayList.mt";

interface Handler {
    function handle(int x): int;
}

class H1 implements Handler {
    public function handle(int x): int { return x + 1; }
}

class H2 implements Handler {
    public function handle(int x): int { return x + 2; }
}

class H3 implements Handler {
    public function handle(int x): int { return x + 3; }
}

class H4 implements Handler {
    public function handle(int x): int { return x + 4; }
}

class H5 implements Handler {
    public function handle(int x): int { return x + 5; }
}

class H6 implements Handler {
    public function handle(int x): int { return x + 6; }
}

function dispatch(ArrayList<Handler> list, int x): int {
    int s = 0;
    for (Handler h : list) {
        s = s + h.handle(x);
    }
    return s;
}

ArrayList<Handler> handlers = new ArrayList<Handler>();
handlers.add(new H1());
handlers.add(new H2());
handlers.add(new H3());
handlers.add(new H4());
handlers.add(new H5());
handlers.add(new H6());

int N = 300000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + dispatch(handlers, i);
}
print("interface_dispatch_collections_hot acc=" + acc);
