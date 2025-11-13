// LinkedListIterator<T> - Iterator for LinkedList<T> collections
// Efficiently iterates through linked nodes without array conversion

import * from "../Iterator.mt";

// Forward declaration of Node class
// Actual Node class should be defined in LinkedList.mt
class LinkedListNode<T> {
    public T data;
    public LinkedListNode<T> next;

    public constructor(T value) {
        this.data = value;
        this.next = null;
    }
}

class LinkedListIterator<T> implements Iterator<T> {
    private LinkedListNode<T> currentNode;

    public constructor(LinkedListNode<T> headNode) {
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
