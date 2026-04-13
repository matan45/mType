// Test recursive generic interface definition
// @Script

import * from "../../lib/primitives/String.mt";

interface Tree<T, N extends TreeNode<T, N>> {
    function getRoot(): N?;
    function add(T value): void;
}

interface TreeNode<T, N extends TreeNode<T, N>> {
    function getValue(): T;
    function getLeft(): N?;
    function getRight(): N?;
    function setLeft(N node): void;
    function setRight(N node): void;
}

class BinaryNode<T> implements TreeNode<T, BinaryNode<T>> {
    private T value;
    private BinaryNode<T>? left;
    private BinaryNode<T>? right;

    public constructor(T value) {
        this.value = value;
        this.left = null;
        this.right = null;
    }

    public function getValue(): T {
        return this.value;
    }

    public function getLeft(): BinaryNode<T>? {
        return this.left;
    }

    public function getRight(): BinaryNode<T>? {
        return this.right;
    }

    public function setLeft(BinaryNode<T> node): void {
        this.left = node;
    }

    public function setRight(BinaryNode<T> node): void {
        this.right = node;
    }
}

class BinaryTree<T> implements Tree<T, BinaryNode<T>> {
    private BinaryNode<T>? root;

    public constructor() {
        this.root = null;
    }

    public function getRoot(): BinaryNode<T>? {
        return this.root;
    }

    public function add(T value): void {
        if (this.root == null) {
            this.root = new BinaryNode<T>(value);
        } else {
            // Simple addition to left or right alternately
            BinaryNode<T> node = new BinaryNode<T>(value);
            if (this.root.getLeft() == null) {
                this.root.setLeft(node);
            } else {
                this.root.setRight(node);
            }
        }
    }
}

BinaryTree<String> tree = new BinaryTree<String>();
tree.add(new String("Root"));
tree.add(new String("Left"));
tree.add(new String("Right"));

BinaryNode<String> root = tree.getRoot();
if (root != null) {
    print("Root: " + root.getValue());

    BinaryNode<String> left = root.getLeft();
    if (left != null) {
        print("Left: " + left.getValue());
    }

    BinaryNode<String> right = root.getRight();
    if (right != null) {
        print("Right: " + right.getValue());
    }
}
