// Test circular references with promises

import { Int } from "../../lib/primitives/Int.mt";

print("=== Async Circular Promise Reference Test ===");

class Node {
    int value;
    Node? next;

    public constructor(int val) {
        this.value = val;
        this.next = null;
    }

    public function getValue(): int {
        return this.value;
    }

    public function setNext(Node? n): void {
        this.next = n;
    }

    public function hasNext(): bool {
        return this.next != null;
    }
}

function async createCircularStructure(): Promise<Node> {
    print("Creating circular structure");
    Node n1 = new Node(1);
    Node n2 = new Node(2);
    Node n3 = new Node(3);

    n1.setNext(n2);
    n2.setNext(n3);
    n3.setNext(n1);  // Circular reference

    print("Circular structure created");
    return n1;
}

function async testCircularReference(): Promise<Int> {
    Node head = await createCircularStructure();

    print("Traversing limited steps...");
    int count = 0;
    int maxSteps = 5;

    Node current = head;
    for (int i = 0; i < maxSteps; i = i + 1) {
        print("Node value: " + current.getValue());
        count = count + 1;

        if (current.hasNext()) {
            // Would continue infinitely due to circular reference
            // but we limit iterations
        }
    }

    print("Traversed " + count + " nodes");
    return new Int(count);
}

function async main(): Promise<Int> {
    Int result = await testCircularReference();
    print("Result: " + result);
    return result;
}

main();
