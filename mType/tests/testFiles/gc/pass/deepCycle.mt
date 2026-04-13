// Test: Deep circular reference chain
// Purpose: Verify that GC can detect cycles longer than A -> B -> A

class ChainNode {
    private ChainNode? next;
    private int id;

    constructor(int nodeId) {
        this.id = nodeId;
        this.next = null;
    }

    public function setNext(ChainNode? n): void {
        this.next = n;
    }

    public function getId(): int {
        return this.id;
    }
}


function main(): void {
    // Create a chain: A -> B -> C -> D -> A
    ChainNode a = new ChainNode(1);
    ChainNode b = new ChainNode(2);
    ChainNode c = new ChainNode(3);
    ChainNode d = new ChainNode(4);

    a.setNext(b);
    b.setNext(c);
    c.setNext(d);
    d.setNext(a);  // Complete the cycle

    // Verify chain is set up
    print(a.getId());
    print(b.getId());
    print(c.getId());
    print(d.getId());

    print("Deep cycle created successfully");
}
main();
