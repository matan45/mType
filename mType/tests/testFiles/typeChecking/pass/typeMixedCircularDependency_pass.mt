// Test circular dependency in type checking - should pass
class Node {
    int value;
    public Node? next;

    constructor(int v) {
        value = v;
        next = null;
    }

    public function setNext(Node? n): void {
        next = n;
    }
}

class LinkedList {
    Node? head;

    constructor() {
        head = null;
    }

    public function add(int value): void {
        Node newNode = new Node(value);
        if (head == null) {
            head = newNode;
        } else {
            Node current = head;
            while (current.next != null) {
                current = current.next;
            }
            current.setNext(newNode);
        }
    }
}

LinkedList list = new LinkedList();
list.add(1);
list.add(2);
list.add(3);
print("Circular dependency test passed");
