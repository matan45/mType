// Test: Casting objects with circular references (memory safety test)
// Tests that casting handles circular references without memory issues

interface INode {
    public function getId(): int;
}

class GraphNode implements INode {
    public int id;
    public GraphNode next;
    public GraphNode prev;

    public constructor(int nodeId) {
        this.id = nodeId;
        this.next = null;
        this.prev = null;
    }

    public function getId(): int {
        return this.id;
    }

    public function setNext(GraphNode n): void {
        this.next = n;
    }

    public function setPrev(GraphNode p): void {
        this.prev = p;
    }
}

class WeightedNode extends GraphNode {
    public int weight;

    public constructor(int nodeId, int w):super(nodeId) {
        this.weight = w;
    }

    public function getWeight(): int {
        return this.weight;
    }
}

// Create circular linked list
print("Creating circular reference structure...");

WeightedNode node1 = new WeightedNode(1, 10);
WeightedNode node2 = new WeightedNode(2, 20);
WeightedNode node3 = new WeightedNode(3, 30);
WeightedNode node4 = new WeightedNode(4, 40);

// Create circular references
node1.setNext(node2);
node2.setNext(node3);
node3.setNext(node4);
node4.setNext(node1);  // Circle back

node1.setPrev(node4);
node2.setPrev(node1);
node3.setPrev(node2);
node4.setPrev(node3);

print("Circular structure created");

// Cast through the circle multiple times
print("Starting circular traversal with casting...");

GraphNode current = node1;
int iterations = 0;
int maxIterations = 20;  // Traverse multiple times through circle

while (iterations < maxIterations) {
    if (current == null) {
        break;
    }

    // Get node ID directly
    int nodeId = current.getId();

    // Try cast to WeightedNode
    WeightedNode wnode = (WeightedNode)current;
    if (wnode != null) {
        int weight = wnode.getWeight();
        // Only print first 4 iterations to keep output clean
        if (iterations < 4) {
            print("Node " + (string)nodeId + " weight: " + (string)weight);
        }
    }

    current = current.next;
    iterations = iterations + 1;
}

print("Completed " + (string)iterations + " iterations");

// Verify we can still access nodes after circular traversal
print("Node1 id: " + (string)node1.id);
print("Node2 id: " + (string)node2.id);
print("Node3 id: " + (string)node3.id);
print("Node4 id: " + (string)node4.id);

// Break circles to help GC (good practice)
node1.setNext(null);
node2.setNext(null);
node3.setNext(null);
node4.setNext(null);
node1.setPrev(null);
node2.setPrev(null);
node3.setPrev(null);
node4.setPrev(null);

print("Test completed successfully");

// Expected output:
// Creating circular reference structure...
// Circular structure created
// Starting circular traversal with casting...
// Node 1 weight: 10
// Node 2 weight: 20
// Node 3 weight: 30
// Node 4 weight: 40
// Completed 20 iterations
// Node1 id: 1
// Node2 id: 2
// Node3 id: 3
// Node4 id: 4
// Test completed successfully
