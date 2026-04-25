// MYT-208: derived class with super(...) call from a stack-promoted ctor.
// Exercises the super-constructor unwind path where the new ctor frame must
// inherit thisInstanceRaw (not thisInstance) so the parent ctor body sees
// the same backing object. Each iteration's Box is bounded by tally's
// frame.

class Base {
    public int v;
    public constructor(int x) {
        this.v = x;
    }
}

class Box extends Base {
    public int w;
    public constructor(int x, int y) : super(x) {
        this.w = y;
    }
}

function tally(int n): int {
    Box b = new Box(n, n + 1);
    return b.v + b.w;
}

int total = 0;
for (int i = 0; i < 4; i = i + 1) {
    total = total + tally(i);
}
print(total);
