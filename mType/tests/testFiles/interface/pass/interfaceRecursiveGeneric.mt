// Test recursive generic interface definition
// @Script

interface Tree<T, N extends TreeNode<T, N>> {
    func getRoot(): N;
    func add(value: T): void;
}

interface TreeNode<T, N extends TreeNode<T, N>> {
    func getValue(): T;
    func getLeft(): N;
    func getRight(): N;
    func setLeft(node: N): void;
    func setRight(node: N): void;
}

class BinaryNode<T> implements TreeNode<T, BinaryNode<T>> {
    var value: T;
    var left: BinaryNode<T>;
    var right: BinaryNode<T>;

    func init(value: T) {
        this.value = value;
        this.left = null;
        this.right = null;
    }

    func getValue(): T {
        return this.value;
    }

    func getLeft(): BinaryNode<T> {
        return this.left;
    }

    func getRight(): BinaryNode<T> {
        return this.right;
    }

    func setLeft(node: BinaryNode<T>): void {
        this.left = node;
    }

    func setRight(node: BinaryNode<T>): void {
        this.right = node;
    }
}

class BinaryTree<T> implements Tree<T, BinaryNode<T>> {
    var root: BinaryNode<T>;

    func init() {
        this.root = null;
    }

    func getRoot(): BinaryNode<T> {
        return this.root;
    }

    func add(value: T): void {
        if (this.root == null) {
            this.root = new BinaryNode<T>(value);
        } else {
            // Simple addition to left or right alternately
            var node = new BinaryNode<T>(value);
            if (this.root.getLeft() == null) {
                this.root.setLeft(node);
            } else {
                this.root.setRight(node);
            }
        }
    }
}

var tree = new BinaryTree<String>();
tree.add("Root");
tree.add("Left");
tree.add("Right");

var root = tree.getRoot();
if (root != null) {
    print("Root: " + root.getValue());

    var left = root.getLeft();
    if (left != null) {
        print("Left: " + left.getValue());
    }

    var right = root.getRight();
    if (right != null) {
        print("Right: " + right.getValue());
    }
}
