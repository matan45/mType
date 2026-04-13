// Test: Force a GC abort by creating a massive graph all at once
// Purpose: Create so many interconnected objects that a single collection exceeds 5ms

class Node {
    private Node? n1;
    private Node? n2;
    private Node? n3;
    private Node? n4;
    private Node? n5;
    private int id;

    constructor(int i) {
        this.id = i;
        this.n1 = null;
        this.n2 = null;
        this.n3 = null;
        this.n4 = null;
        this.n5 = null;
    }

    public function setN1(Node? n): void { this.n1 = n; }
    public function setN2(Node? n): void { this.n2 = n; }
    public function setN3(Node? n): void { this.n3 = n; }
    public function setN4(Node? n): void { this.n4 = n; }
    public function setN5(Node? n): void { this.n5 = n; }
}

function main(): void {
    int size = 50000;
    Node[] nodes = new Node[size];

    // Create all nodes first
    int i = 0;
    while (i < size) {
        nodes[i] = new Node(i);
        i = i + 1;
    }

    print("Created " + size + " nodes");

    // Now link them all together with 5 references each
    i = 0;
    while (i < size) {
        nodes[i].setN1(nodes[(i + 1) % size]);
        nodes[i].setN2(nodes[(i + size/5) % size]);
        nodes[i].setN3(nodes[(i + size/4) % size]);
        nodes[i].setN4(nodes[(i + size/3) % size]);
        nodes[i].setN5(nodes[(i + size/2) % size]);
        i = i + 1;
    }

    print("Linked " + size + " nodes with 5 references each");
    print("Total edges: " + (size * 5));
    print("GC will run at program end...");
}

main();
