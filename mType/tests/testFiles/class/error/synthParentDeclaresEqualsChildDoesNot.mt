// MYT-274: when a parent declares its own equals/hashCode and the child does
// not override BOTH, auto-synthesis is rejected with InheritanceException
// because the child's auto-synthesized contract would silently disagree
// with the parent's hand-written contract.

class Base {
    private int tag;

    public constructor(int t) {
        this.tag = t;
    }

    public function equals(Object other): bool {
        return false;
    }

    public function hashCode(): int {
        return 42;
    }
}

class Derived extends Base {
    private int extra;

    public constructor(int t, int e) {
        super(t);
        this.extra = e;
    }
}

Derived d = new Derived(1, 2);
print(d.hashCode());
