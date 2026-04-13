// Test: Casting with very large object graph (memory & performance test)
// Tests that the VM can handle casting large interconnected object graphs

class Node {
    public int value;
    public Node? left;
    public Node? right;
    public Node? parent;

    public constructor(int v) {
        this.value = v;
        this.left = null;
        this.right = null;
        this.parent = null;
    }

    public function setValue(int v): void {
        this.value = v;
    }
}

class TreeNode extends Node {
    public int depth;

    public constructor(int v, int d):super(v) {
        this.depth = d;
    }

    public function getDepth(): int {
        return this.depth;
    }
}

// Create a large binary tree (depth 8 = 255 nodes)
function createTree(int depth, int currentDepth, TreeNode? parent): TreeNode? {
    if (currentDepth > depth) {
        return null;
    }

    TreeNode node = new TreeNode(currentDepth * 100, currentDepth);
    node.parent = parent;

    if (currentDepth < depth) {
        node.left = createTree(depth, currentDepth + 1, node);
        node.right = createTree(depth, currentDepth + 1, node);
    }

    return node;
}

// Traverse tree and cast nodes
function traverseAndCast(TreeNode node, int count): int {
    if (node == null) {
        return count;
    }

    // Cast TreeNode to Node (upcast)
    Node baseNode = (Node)node;
    baseNode.setValue(baseNode.value + 1);

    // Cast back to TreeNode (downcast)
    TreeNode treeNode = (TreeNode)baseNode;
    if (treeNode != null) {
        count = count + 1;
    }

    count = traverseAndCast((TreeNode)node.left, count);
    count = traverseAndCast((TreeNode)node.right, count);

    return count;
}

print("Creating large object graph...");
TreeNode root = createTree(7, 0, null);
print("Tree created");

print("Starting traversal with casting...");
int nodeCount = traverseAndCast(root, 0);
print("Traversed and cast " + (string)nodeCount + " nodes");

// Verify root value was modified
if (root != null) {
    print("Root value: " + (string)root.value);
    print("Root depth: " + (string)root.depth);
}

print("Test completed successfully");

// Expected output:
// Creating large object graph...
// Tree created
// Starting traversal with casting...
// Traversed and cast 255 nodes
// Root value: 1
// Root depth: 0
// Test completed successfully
