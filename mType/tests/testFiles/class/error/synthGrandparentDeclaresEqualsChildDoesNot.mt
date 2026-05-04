// MYT-274: parent-conflict rule walks the full ancestor chain. When the
// conflict is in the grandparent (not the immediate parent), the error
// message must say "transitively inherits from" rather than "inherits
// from", since the latter implies direct inheritance.

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

class Middle extends Base {
    public constructor(int t) {
        super(t);
    }
}

class Leaf extends Middle {
    private int extra;

    public constructor(int t, int e) {
        super(t);
        this.extra = e;
    }
}

Leaf l = new Leaf(1, 2);
print(l.hashCode());
