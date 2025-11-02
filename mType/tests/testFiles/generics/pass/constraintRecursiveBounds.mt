import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Recursive bounds in generic constraints
interface Tree<T extends Tree<T>> {
    function getLeft(): T;
    function getRight(): T;
    function getValue(): String;
}

class BinaryTree extends Tree<BinaryTree> {
    String value;
    BinaryTree left;
    BinaryTree right;

    public function BinaryTree(String v) {
        value = v;
    }

    public function setLeft(BinaryTree l): void {
        left = l;
    }

    public function setRight(BinaryTree r): void {
        right = r;
    }

    public function getLeft(): BinaryTree {
        return left;
    }

    public function getRight(): BinaryTree {
        return right;
    }

    public function getValue(): String {
        return value;
    }
}

class TreeProcessor<T extends Tree<T>> {
    T root;

    public function setRoot(T r): void {
        root = r;
    }

    public function printRoot(): void {
        print("Root: " + root.getValue());
    }
}

function main(): void {
    BinaryTree tree = new BinaryTree(new String("Root"));
    tree.setLeft(new BinaryTree(new String("Left")));
    tree.setRight(new BinaryTree(new String("Right")));

    TreeProcessor<BinaryTree> processor = new TreeProcessor<BinaryTree>();
    processor.setRoot(tree);
    processor.printRoot();
}

main();
