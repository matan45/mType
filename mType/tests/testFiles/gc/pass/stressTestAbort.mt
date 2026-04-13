// Test: Stress test to trigger GC abort due to time limit
// Purpose: Create many interconnected objects to exceed MAX_CYCLE_DETECTION_TIME_MS (5ms)

class Node {
    private Node? next;
    private Node? prev;
    private int id;

    constructor(int i) {
        this.id = i;
        this.next = null;
        this.prev = null;
    }

    public function setNext(Node? n): void {
        this.next = n;
    }

    public function setPrev(Node? n): void {
        this.prev = n;
    }

    public function getId(): int {
        return this.id;
    }
}

function createLargeGraph(int size): void {
    // Create an array to hold all nodes
    Node[] nodes = new Node[size];

    // Create all nodes
    int i = 0;
    while (i < size) {
        nodes[i] = new Node(i);
        i = i + 1;
    }

    // Create a complex web of references (each node references several others)
    i = 0;
    while (i < size) {
        // Each node points to the next one (circular)
        int nextIdx = (i + 1) % size;
        nodes[i].setNext(nodes[nextIdx]);

        // Each node also points back (doubly-linked circular)
        int prevIdx = (i - 1 + size) % size;
        nodes[i].setPrev(nodes[prevIdx]);

        i = i + 1;
    }

    print("Created graph with " + size + " nodes");
    print("Each node has 2 references (next + prev)");
    print("Total reference edges: " + (size * 2));
}

function main(): void {
    // Create a large interconnected graph
    // 5000 nodes with 2 references each = 10000 reference edges to traverse
    // This should take more than 5ms to fully scan
    createLargeGraph(5000);

    print("Graph created - GC will attempt collection at program end");
    print("If collection takes > 5ms, it will be aborted");
}

main();
