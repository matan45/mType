// Test: Large stress test to trigger GC abort due to time limit
// Purpose: Create many interconnected objects to exceed MAX_CYCLE_DETECTION_TIME_MS (5ms)

class Node {
    private Node next;
    private Node prev;
    private Node cross1;
    private Node cross2;
    private int id;

    constructor(int i) {
        this.id = i;
        this.next = null;
        this.prev = null;
        this.cross1 = null;
        this.cross2 = null;
    }

    public function setNext(Node n): void {
        this.next = n;
    }

    public function setPrev(Node n): void {
        this.prev = n;
    }

    public function setCross1(Node n): void {
        this.cross1 = n;
    }

    public function setCross2(Node n): void {
        this.cross2 = n;
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

    // Create a complex web of references (each node references 4 others)
    i = 0;
    while (i < size) {
        // Each node points to the next one (circular)
        int nextIdx = (i + 1) % size;
        nodes[i].setNext(nodes[nextIdx]);

        // Each node also points back (doubly-linked circular)
        int prevIdx = (i - 1 + size) % size;
        nodes[i].setPrev(nodes[prevIdx]);

        // Cross-references to create a more complex graph
        int cross1Idx = (i + size / 3) % size;
        nodes[i].setCross1(nodes[cross1Idx]);

        int cross2Idx = (i + size / 2) % size;
        nodes[i].setCross2(nodes[cross2Idx]);

        i = i + 1;
    }

    print("Created graph with " + size + " nodes");
    print("Each node has 4 references");
    print("Total reference edges: " + (size * 4));
}

function main(): void {
    // Create a very large interconnected graph
    // 20000 nodes with 4 references each = 80000 reference edges to traverse
    createLargeGraph(20000);

    print("Graph created - GC will attempt collection at program end");
    print("If collection takes > 5ms, it will be aborted");
}

main();
