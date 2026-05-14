// TARGET: object/method-chain coverage without collection library imports.
// Builds a short linked chain and repeatedly traverses next/value methods.

class ChainNode {
    public int value;
    public ChainNode next;

    public constructor(int value) {
        this.value = value;
        this.next = null;
    }

    public function setNext(ChainNode n): void {
        this.next = n;
    }

    public function nextNode(): ChainNode {
        return this.next;
    }

    public function getValue(): int {
        return this.value;
    }
}

class Chain {
    public ChainNode head;

    public constructor(ChainNode head) {
        this.head = head;
    }

    public function sumValues(): int {
        int total = 0;
        ChainNode cur = this.head;
        while (cur != null) {
            total = total + cur.getValue();
            cur = cur.nextNode();
        }
        return total;
    }
}

ChainNode n0 = new ChainNode(0);
ChainNode n1 = new ChainNode(1);
ChainNode n2 = new ChainNode(2);
ChainNode n3 = new ChainNode(3);
ChainNode n4 = new ChainNode(4);
ChainNode n5 = new ChainNode(5);
ChainNode n6 = new ChainNode(6);
ChainNode n7 = new ChainNode(7);
n0.setNext(n1);
n1.setNext(n2);
n2.setNext(n3);
n3.setNext(n4);
n4.setNext(n5);
n5.setNext(n6);
n6.setNext(n7);

Chain chain = new Chain(n0);
int N = 20000;
int total = 0;
for (int i = 0; i < N; i = i + 1) {
    total = total + chain.sumValues();
}

print("method_chain_hot total=" + total);
