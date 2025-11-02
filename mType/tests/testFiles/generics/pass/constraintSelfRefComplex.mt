import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Complex self-referential constraints
interface Node<T extends Node<T>> {
    function getNext(): T;
    function setNext(T next): void;
}

class LinkedNode extends Node<LinkedNode> {
    String data;
    LinkedNode next;

    public function LinkedNode(String d) {
        data = d;
    }

    public function getNext(): LinkedNode {
        return next;
    }

    public function setNext(LinkedNode n): void {
        next = n;
    }

    public function getData(): String {
        return data;
    }
}

class Chain<T extends Node<T>> {
    T head;

    public function setHead(T h): void {
        head = h;
    }

    public function getHead(): T {
        return head;
    }
}

function main(): void {
    LinkedNode node1 = new LinkedNode(new String("First"));
    LinkedNode node2 = new LinkedNode(new String("Second"));
    node1.setNext(node2);

    Chain<LinkedNode> chain = new Chain<LinkedNode>();
    chain.setHead(node1);

    print(chain.getHead().getData());
    print(chain.getHead().getNext().getData());
}

main();
