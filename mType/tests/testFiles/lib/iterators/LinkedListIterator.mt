// LinkedListIterator<T> - Iterator for LinkedList<T> collections
// Efficiently iterates through linked nodes without array conversion

import * from "../Iterator.mt";

// Forward declaration of Node class matching LinkedList.mt
// Actual Node class is defined in LinkedList.mt
class Node<T> {
    public T data;
    public Node<T> next;
    public Node<T> prev;

    constructor(T data) {
        this.data = data;
        this.next = null;
        this.prev = null;
    }

    constructor(T data, Node<T> prev, Node<T> next) {
        this.data = data;
        this.prev = prev;
        this.next = next;
    }
}

class LinkedListIterator<T> implements Iterator<T> {
    private Node<T> currentNode;

    public constructor(Node<T> headNode) {
        this.currentNode = headNode;
    }

    public function hasNext(): bool {
        return this.currentNode != null;
    }

    public function next(): T {
        if (!this.hasNext()) {
            print("Error: No more elements in iterator");
            return null;
        }
        T element = this.currentNode.data;
        this.currentNode = this.currentNode.next;
        return element;
    }

    public function close(): void {
        // No resources to clean up
    }
}
