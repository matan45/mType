class Node {
    public int value;
    public Node? next;

    public constructor(int val) {
        value = val;
        next = null;
    }

    public function hasNext(): bool {
        if (next == null) {
            return false;
        }
        return true;
    }
}

Node n1 = new Node(10);
print(n1.value); // 10
print(n1.hasNext()); // false

Node n2 = new Node(20);
n1.next = n2;
print(n1.hasNext()); // true

Node? n3 = null;
print(n3); // null

if (n3 == null) {
    print(999); // Should print
}

n3 = new Node(30);
if (n3 != null) {
    print(n3.value); // 30
}