// Combo 09: Recursive + Async + Generics + Error Handling

import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/ArrayList.mt";

class TreeException extends Exception {
    public constructor(string msg) : super(msg) {}
}

class TreeNode<T> {
    private T value;
    private ArrayList<TreeNode<T>> children;
    private bool corrupted;

    public constructor(T value) {
        this.value = value;
        this.children = new ArrayList<TreeNode<T>>();
        this.corrupted = false;
    }

    public constructor(T value, bool corrupted) {
        this.value = value;
        this.children = new ArrayList<TreeNode<T>>();
        this.corrupted = corrupted;
    }

    public function getValue(): T { return this.value; }
    public function isCorrupted(): bool { return this.corrupted; }

    public function addChild(TreeNode<T> child): void {
        this.children.add(child);
    }

    public function getChildren(): ArrayList<TreeNode<T>> {
        return this.children;
    }
}

// Recursive async with generic type + error handling
function async <T> traverseTree(TreeNode<T> node, int depth): Promise<Int> {
    if (node.isCorrupted()) {
        throw new TreeException("Corrupted node at depth " + depth + ": " + node.getValue());
    }

    string indent = "";
    for (int i = 0; i < depth; i++) {
        indent = indent + "  ";
    }
    print(indent + "Visit: " + node.getValue());

    int count = 1;
    ArrayList<TreeNode<T>> children = node.getChildren();

    for (int i = 0; i < children.size(); i++) {
        try {
            Int childCount = await traverseTree<T>(children.get(i), depth + 1);
            count = count + childCount.getValue();
        } catch (TreeException e) {
            print(indent + "  SKIP: " + e.getMessage());
        }
    }

    return new Int(count);
}

function async factorial(int n): Promise<Int> {
    if (n <= 1) {
        return new Int(1);
    }
    Int subResult = await factorial(n - 1);
    return new Int(n * subResult.getValue());
}

function async main(): Promise<void> {
    print("=== Combo 09: Recursive + Async + Generics + Error ===");

    // Build generic tree with boxed String
    TreeNode<String> root = new TreeNode<String>(new String("root"));
    TreeNode<String> a = new TreeNode<String>(new String("A"));
    TreeNode<String> b = new TreeNode<String>(new String("B"));
    TreeNode<String> c = new TreeNode<String>(new String("C"), true);

    TreeNode<String> a1 = new TreeNode<String>(new String("A1"));
    TreeNode<String> a2 = new TreeNode<String>(new String("A2"));
    TreeNode<String> b1 = new TreeNode<String>(new String("B1"));
    TreeNode<String> b2 = new TreeNode<String>(new String("B2"), true);

    a.addChild(a1);
    a.addChild(a2);
    b.addChild(b1);
    b.addChild(b2);
    root.addChild(a);
    root.addChild(b);
    root.addChild(c);

    print("--- Tree traversal ---");
    Int visited = await traverseTree<String>(root, 0);
    print("Total visited: " + visited.getValue());

    print("--- Async factorial ---");
    Int f5 = await factorial(5);
    print("5! = " + f5.getValue());

    Int f10 = await factorial(10);
    print("10! = " + f10.getValue());

    // Int tree
    print("--- Int tree ---");
    TreeNode<Int> intRoot = new TreeNode<Int>(new Int(1));
    intRoot.addChild(new TreeNode<Int>(new Int(2)));
    intRoot.addChild(new TreeNode<Int>(new Int(3)));

    Int intVisited = await traverseTree<Int>(intRoot, 0);
    print("Int tree visited: " + intVisited.getValue());

    print("=== Combo 09 Complete ===");
}

main();
