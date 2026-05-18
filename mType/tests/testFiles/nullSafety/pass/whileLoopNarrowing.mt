// Test: `while (x != null) { ... }` narrows x to non-null inside the body.

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

Node third = new Node(3);
Node second = new Node(2);
second.next = third;
Node head = new Node(1);
head.next = second;

Node? current = head;
int sum = 0;
while (current != null) {
    sum = sum + current.getValue();
    current = current.getNext();
}
print("sum: " + sum);

Node? emptyHead = null;
int emptySum = 0;
while (emptyHead != null) {
    emptySum = emptySum + emptyHead.getValue();
    emptyHead = emptyHead.getNext();
}
print("empty sum: " + emptySum);

print("While loop narrowing tests passed!");
