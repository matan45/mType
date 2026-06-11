// Test: a 50-level self-referential object chain serializes and
// deserializes without losing depth (recursion safety in both directions).
import * from "../../lib/json/Json.mt";

class Node {
    public int depth;
    public Node? child;

    public constructor() {
        this.depth = 0;
        this.child = null;
    }
}

function main(): void {
    Node root = new Node();
    root.depth = 0;
    Node current = root;
    for (int i = 1; i < 50; i++) {
        Node next = new Node();
        next.depth = i;
        current.child = next;
        current = next;
    }

    string json = Json::serialize(root);
    Node restored = Json::deserializeAs(json, "Node");

    int depth = 0;
    Node? walker = restored;
    int lastDepth = -1;
    while (walker != null) {
        lastDepth = walker.depth;
        depth = depth + 1;
        walker = walker.child;
    }
    print("levels: " + depth);
    print("last depth: " + lastDepth);
    print("Deep nesting round trip test passed!");
}
main();
