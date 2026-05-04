// MYT-274: synthesized equals(Object) preserves the equals/hashCode contract
// (reflexive, symmetric, transitive, consistent with hashCode).

class Pair {
    private int a;
    private int b;

    public constructor(int a, int b) {
        this.a = a;
        this.b = b;
    }
}

Pair p = new Pair(1, 2);
Pair q = new Pair(1, 2);
Pair r = new Pair(1, 2);
Pair s = new Pair(2, 1);

print("reflexive: " + p.equals(p));
print("symmetric pq: " + p.equals(q));
print("symmetric qp: " + q.equals(p));
print("transitive pr (via q): " + (p.equals(q) && q.equals(r) && p.equals(r)));
print("differing not equal: " + p.equals(s));
print("hash matches when equal: " + (p.hashCode() == q.hashCode()));
