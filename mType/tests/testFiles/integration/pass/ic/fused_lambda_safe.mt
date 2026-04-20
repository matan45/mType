// MYT-198: fusion must NOT fire inside a lambda body. LOAD_LOCAL in a lambda
// frame has captured-variable semantics that the fused executor doesn't
// replicate (it reads directly from localBase + slot). The fusion trigger
// guards on callStack.back().originatingLambda / sharedFrame; this test hits
// that path by running a hot method call through a lambda body.

class Widget {
    public int count;
    public constructor() {
        this.count = 0;
    }
    public function bump(int step): int {
        this.count = this.count + step;
        return this.count;
    }
}

Widget w = new Widget();

function apply(function(int): int fn): int {
    int r = 0;
    for (int i = 0; i < 500; i = i + 1) {
        r = r + fn(i);
    }
    return r;
}

int r = apply((int x) => w.bump(x));
print(r);
print(w.count);
