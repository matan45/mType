// Test: Doubly-linked circular list creating cycles
// Purpose: Verify that GC handles circular prev/next pointer cycles

class ListNode {
    private string value;
    private ListNode? next;
    private ListNode? prev;

    constructor(string v) {
        this.value = v;
        this.next = null;
        this.prev = null;
    }

    public function setNext(ListNode? n): void {
        this.next = n;
    }

    public function setPrev(ListNode? p): void {
        this.prev = p;
    }

    public function getValue(): string {
        return this.value;
    }
}

function main(): void {
    // Create three nodes
    ListNode a = new ListNode("A");
    ListNode b = new ListNode("B");
    ListNode c = new ListNode("C");

    // Link them in a circular doubly-linked pattern: A <-> B <-> C <-> A
    a.setNext(b);
    b.setPrev(a);

    b.setNext(c);
    c.setPrev(b);

    c.setNext(a);
    a.setPrev(c);

    // Verify the nodes are set up correctly
    print(a.getValue());
    print(b.getValue());
    print(c.getValue());

    print("Linked list cycle created successfully");
}
main();
