// Test circular references in object arrays
print("Testing circular references");

class Node {
    int value;
    Node next;

    Node(int v) {
        value = v;
        next = null;
    }

    void setNext(Node n) {
        next = n;
    }

    int getValue() {
        return value;
    }
}

Node[] nodes = new Node[3];
nodes[0] = new Node(1);
nodes[1] = new Node(2);
nodes[2] = new Node(3);

// Create circular reference
nodes[0].setNext(nodes[1]);
nodes[1].setNext(nodes[2]);
nodes[2].setNext(nodes[0]);

print("Circular linked list in array:");
Node current = nodes[0];
for (int i = 0; i < 6; i++) {
    print("  Node value: " + current.getValue());
    current = current.next;
}

print("Circular reference test completed");
