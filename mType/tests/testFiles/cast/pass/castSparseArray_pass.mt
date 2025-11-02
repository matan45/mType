// Test sparse array casting
// Demonstrates casting arrays with many uninitialized (null) elements

class Node {
    int value;
    string label;
}

class TreeNode extends Node {
    TreeNode left;
    TreeNode right;

    constructor(int val) {
        value = val;
        label = "TreeNode";
    }
}

@Script
function testSparseArrayCasting(): void {
    print("Testing sparse array casting");

    // Create large sparse array
    TreeNode[] sparse = new TreeNode[10];

    // Only initialize a few elements
    sparse[0] = new TreeNode(100);
    sparse[3] = new TreeNode(103);
    sparse[7] = new TreeNode(107);
    sparse[9] = new TreeNode(109);

    print("Sparse array length: " + sparse.length);

    // Upcast to Node array
    Node[] nodes = sparse;

    // Count non-null elements
    int nonNullCount = 0;
    for (int i = 0; i < nodes.length; i = i + 1) {
        if (nodes[i] != null) {
            nonNullCount = nonNullCount + 1;
        }
    }
    print("Non-null elements: " + nonNullCount);

    // Access and cast specific non-null elements
    if (nodes[0] != null) {
        TreeNode tn = (TreeNode)nodes[0];
        print("TreeNode at [0] value: " + tn.value);
    }

    if (nodes[3] != null) {
        TreeNode tn = (TreeNode)nodes[3];
        print("TreeNode at [3] value: " + tn.value);
    }

    if (nodes[7] != null) {
        TreeNode tn = (TreeNode)nodes[7];
        print("TreeNode at [7] value: " + tn.value);
    }

    if (nodes[9] != null) {
        TreeNode tn = (TreeNode)nodes[9];
        print("TreeNode at [9] value: " + tn.value);
    }

    // Verify null elements remain null
    if (nodes[1] == null) {
        print("Element at [1] is null");
    }
    if (nodes[5] == null) {
        print("Element at [5] is null");
    }

    print("Sparse array casting completed");
}
