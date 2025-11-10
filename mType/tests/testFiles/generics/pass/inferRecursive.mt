import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Recursive type inference
class Node<T> {
    T data;
    Node<T> next;

    public function setData(T d): void {
        data = d;
    }

    public function setNext(Node<T> n): void {
        next = n;
    }

    public function getData(): T {
        return data;
    }

    public function getNext(): Node<T> {
        return next;
    }
}

function <T> createChain(T first, T second): Node<T> {
    Node<T> node1 = new Node<T>();
    node1.setData(first);

    Node<T> node2 = new Node<T>();
    node2.setData(second);

    node1.setNext(node2);
    return node1;
}

function main(): void {
    // Recursive type inference for linked structure
    Node<String> chain = createChain<String>(new String("Start"), new String("End"));
    print(chain.getData().toString());
    print(chain.getNext().getData().toString());
}

main();
