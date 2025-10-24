// Test circular generic dependency error
class Node<T> {
    T value;
    Node<Node<T>> next;  // Circular nesting

    public function Node(T val) {
        value = val;
        next = null;
    }
}

class SelfReferencing<T extends SelfReferencing<T>> {
    T value;
}

function main(): void {
    // ERROR: This creates complex circular dependencies
    Node<int> node = new Node<int>(42);
    SelfReferencing<int> self = new SelfReferencing<int>();

    print("This should not execute");
}

main();
