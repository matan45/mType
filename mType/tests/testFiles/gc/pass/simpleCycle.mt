// Test: Simple circular reference between two objects
// Purpose: Verify that GC can detect and handle A -> B -> A cycles

class Node {
    private Node next;
    private string name;

    constructor(string n) {
        this.name = n;
        this.next = null;
    }

    public function setNext(Node n): void {
        this.next = n;
    }

    public function getName(): string {
        return this.name;
    }
}


function main(): void {
    // Create two nodes that reference each other
    Node a = new Node("A");
    Node b = new Node("B");

    // Create a cycle: A -> B -> A
    a.setNext(b);
    b.setNext(a);

    // Verify the cycle is set up correctly
    print(a.getName());
    print(b.getName());

    // When this function returns, both a and b should become
    // unreachable from roots. The GC should detect the cycle
    // and eventually reclaim them.
    print("Cycle created successfully");
}
main();
