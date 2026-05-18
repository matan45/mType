// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises method dispatch on a leaf instance of a 6-level inheritance chain.
// Each level introduces a unique method that the leaf inherits unchanged, so
// dispatch must walk the inherited method table at registration time and the
// hot path benefits from method-index hashing in ClassDefinition.

class L0 {
    public function v0(int x): int { return x + 1; }
}

class L1 extends L0 {
    public function v1(int x): int { return x + 2; }
}

class L2 extends L1 {
    public function v2(int x): int { return x + 3; }
}

class L3 extends L2 {
    public function v3(int x): int { return x + 4; }
}

class L4 extends L3 {
    public function v4(int x): int { return x + 5; }
}

class L5 extends L4 {
    public function v5(int x): int { return x + 6; }
}

L5 leaf = new L5();
int N = 500000;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    acc = acc + leaf.v0(i) + leaf.v1(i) + leaf.v2(i) + leaf.v3(i) + leaf.v4(i) + leaf.v5(i);
}
print("deep_inheritance_hot acc=" + acc);
