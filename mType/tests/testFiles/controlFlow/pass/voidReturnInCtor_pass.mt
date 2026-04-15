// MYT-112: bare `return;` inside a constructor is a legitimate early-exit.
// The ctor's return type is implicitly the declaring-class instance, which
// is returned by the bytecode (LOAD_LOCAL 0 + RETURN_VALUE) on the explicit
// `return;` just as it is at the end of the body.

class Box {
    public int x;

    public constructor(int seed) {
        if (seed < 0) {
            this.x = 0;
            return;
        }
        this.x = seed;
    }
}

Box a = new Box(-1);
Box b = new Box(42);
print(a.x);
print(b.x);
