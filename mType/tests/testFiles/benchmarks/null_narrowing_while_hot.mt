// MYT-373: while-loop null-narrowing on a hot path.
// Path under test: `while (current != null)` narrows `current` to non-null
// inside the loop body (ControlFlowCompiler_Loops.cpp), so current.getValue()
// and current.getNext() dispatch on the nullable Node? receiver. Complements
// null_narrowing_hot (&& / if) and null_guard_clause_hot (|| / guard) to cover
// the loop-condition branch of the narrowing matrix.
// TARGET: ~2s on dev machine (Release). Adjust M if outside 1-5s.

class Node {
    public int value;
    public Node? next;

    constructor(int v) {
        this.value = v;
        this.next = null;
    }

    public function getValue(): int {
        return this.value;
    }

    public function getNext(): Node? {
        return this.next;
    }
}

// Build a chain of L nodes once: values L, L-1, ..., 1 (sum = L*(L+1)/2).
int L = 50;
Node? head = null;
for (int k = 0; k < L; k = k + 1) {
    Node n = new Node(k + 1);
    n.next = head;
    head = n;
}

// Traverse the chain M times; every walk relies on while-condition narrowing.
int M = 80000;
int acc = 0;
for (int i = 0; i < M; i = i + 1) {
    Node? current = head;
    while (current != null) {
        acc = acc + current.getValue();
        current = current.getNext();
    }
}

print("null_narrowing_while_hot acc=" + acc);
