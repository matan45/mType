// Test sparse array casting
// Demonstrates casting arrays with many uninitialized (null) elements

class Node {
    public int value;
    public string label;
}

class TreeNode extends Node {
    TreeNode left;
    TreeNode right;

    constructor(int val): super() {
        value = val;
        label = "TreeNode";
    }
}


function testSparseArrayCasting(): void {
    print("Testing sparse array casting");

    // Create large sparse Node array and populate with TreeNode instances
    // Individual element assignment allows upcasting TreeNode -> Node
    Node[] nodes = new Node[10];

    // Only initialize a few elements
    nodes[0] = new TreeNode(100);
    nodes[3] = new TreeNode(103);
    nodes[7] = new TreeNode(107);
    nodes[9] = new TreeNode(109);

    print("Sparse array length: " + nodes.length);

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

testSparseArrayCasting();
