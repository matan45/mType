import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test self-referential generic types (Tree<T> contains Tree<T> children)
class TreeNode<T> {
    T value;
    TreeNode<T> left;
    TreeNode<T> right;

    constructor(T val) {
        value = val;
        left = null;
        right = null;
    }

    public function setValue(T val): void {
        value = val;
    }

    public function getValue(): T {
        return value;
    }

    public function setLeft(TreeNode<T> node): void {
        left = node;
    }

    public function setRight(TreeNode<T> node): void {
        right = node;
    }

    public function getLeft(): TreeNode<T> {
        return left;
    }

    public function getRight(): TreeNode<T> {
        return right;
    }

    public function hasLeft(): bool {
        return left != null;
    }

    public function hasRight(): bool {
        return right != null;
    }
}

function main(): void {
    print("Testing generic tree structure");

    // Create a binary tree of integers
    TreeNode<Int> root = new TreeNode<Int>(new Int(50));
    TreeNode<Int> leftChild = new TreeNode<Int>(new Int(25));
    TreeNode<Int> rightChild = new TreeNode<Int>(new Int(75));

    root.setLeft(leftChild);
    root.setRight(rightChild);

    print("Tree root value: " + root.getValue().toString());
    print("Tree has left child: " + root.hasLeft());
    print("Tree has right child: " + root.hasRight());
    print("Left child value: " + root.getLeft().getValue().toString());
    print("Right child value: " + root.getRight().getValue().toString());

    // Add grandchildren
    TreeNode<Int> leftLeft = new TreeNode<Int>(new Int(12));
    TreeNode<Int> leftRight = new TreeNode<Int>(new Int(37));
    leftChild.setLeft(leftLeft);
    leftChild.setRight(leftRight);

    print("Left-left grandchild value: " + root.getLeft().getLeft().getValue().toString());
    print("Left-right grandchild value: " + root.getLeft().getRight().getValue().toString());

    // Create a tree of strings
    TreeNode<String> stringRoot = new TreeNode<String>(new String("root"));
    TreeNode<String> stringLeft = new TreeNode<String>(new String("left"));
    TreeNode<String> stringRight = new TreeNode<String>(new String("right"));

    stringRoot.setLeft(stringLeft);
    stringRoot.setRight(stringRight);

    print("String tree root: " + stringRoot.getValue().toString());
    print("String tree left: " + stringRoot.getLeft().getValue().toString());
    print("String tree right: " + stringRoot.getRight().getValue().toString());

    print("Generic tree structure test completed");
}

main();
