// MYT-164 F-b: depth-2 nested inlining. Scaler.compute calls
// Doubler.twice via a pre-existing field (keeps compute's op count
// small — no NEW_OBJECT inside the body). When the outer inliner
// pushes compute's frame, the inner CALL_METHOD at twice's site
// finds s.inlineStack.size() == 1, passes the INLINE_DEPTH_LIMIT = 2
// check in InlineAnalysis, and inlines a second frame whose
// localsBaseSlot stacks past compute's locals. Correctness check:
// sum_{i=0..99}(2*i + 1) = 10000.

class Doubler {
    public function twice(int x): int {
        return x + x;
    }
}

class Scaler {
    public Doubler d;
    public constructor() {
        this.d = new Doubler();
    }
    public function compute(int x): int {
        return this.d.twice(x) + 1;
    }
}

Scaler s = new Scaler();
int total = 0;
for (int i = 0; i < 100; i = i + 1) {
    total = total + s.compute(i);
}
print(total);
