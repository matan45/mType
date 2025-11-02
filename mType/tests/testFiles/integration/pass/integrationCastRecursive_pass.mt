// Integration Test: Casting in recursive functions
// Tests casting behavior in recursive call scenarios

class TreeNode {
    public int value;
    public TreeNode left;
    public TreeNode right;

    public constructor(int v) {
        this.value = v;
        this.left = null;
        this.right = null;
    }

    public function getType(): string {
        return "TreeNode";
    }
}

class BinarySearchNode extends TreeNode {
    public bool isRed;

    public constructor(int v, bool red) {
        super(v);
        this.isRed = red;
    }

    public function getType(): string {
        return "BinarySearchNode";
    }

    public function getColor(): string {
        if (this.isRed) {
            return "red";
        }
        return "black";
    }
}

class AVLNode extends TreeNode {
    public int height;

    public constructor(int v, int h) {
        super(v);
        this.height = h;
    }

    public function getType(): string {
        return "AVLNode";
    }

    public function getHeight(): int {
        return this.height;
    }
}

// Test 1: Recursive traversal with casting
print("Test 1: Recursive traversal with casting");

function recursiveInOrder(TreeNode node): void {
    if (node == null) {
        return;
    }

    // Traverse left
    recursiveInOrder(node.left);

    // Process current node with casting
    if (node isClassOf BinarySearchNode) {
        BinarySearchNode bsn = (BinarySearchNode)node;
        print("BSN: " + bsn.value + " (color: " + bsn.getColor() + ")");
    } else if (node isClassOf AVLNode) {
        AVLNode avl = (AVLNode)node;
        print("AVL: " + avl.value + " (height: " + avl.getHeight() + ")");
    } else {
        print("Node: " + node.value);
    }

    // Traverse right
    recursiveInOrder(node.right);
}

// Build a tree
TreeNode root = new BinarySearchNode(10, true);
root.left = new BinarySearchNode(5, false);
root.right = new AVLNode(15, 2);

recursiveInOrder(root);

// Test 2: Recursive sum with conditional casting
print("Test 2: Recursive sum with conditional casting");

function recursiveSum(TreeNode node, bool doubleRed): int {
    if (node == null) {
        return 0;
    }

    int currentValue = node.value;

    // Double the value if it's a red node and flag is set
    if (doubleRed && node isClassOf BinarySearchNode) {
        BinarySearchNode bsn = (BinarySearchNode)node;
        if (bsn.isRed) {
            currentValue = currentValue * 2;
        }
    }

    int leftSum = recursiveSum(node.left, doubleRed);
    int rightSum = recursiveSum(node.right, doubleRed);

    return currentValue + leftSum + rightSum;
}

int total = recursiveSum(root, true);
print("Total sum with doubling red: " + total);

int regularTotal = recursiveSum(root, false);
print("Total sum regular: " + regularTotal);

// Test 3: Recursive depth calculation with casting
print("Test 3: Recursive depth with type-specific logic");

function recursiveDepth(TreeNode node, int currentDepth): int {
    if (node == null) {
        return currentDepth;
    }

    int adjustment = 0;

    // AVL nodes add their height to depth calculation
    if (node isClassOf AVLNode) {
        AVLNode avl = (AVLNode)node;
        adjustment = avl.getHeight();
    }

    int leftDepth = recursiveDepth(node.left, currentDepth + 1 + adjustment);
    int rightDepth = recursiveDepth(node.right, currentDepth + 1 + adjustment);

    if (leftDepth > rightDepth) {
        return leftDepth;
    }
    return rightDepth;
}

int depth = recursiveDepth(root, 0);
print("Tree depth: " + depth);

// Test 4: Recursive search with casting and early return
print("Test 4: Recursive search with casting");

function recursiveSearch(TreeNode node, int target): bool {
    if (node == null) {
        return false;
    }

    // Check if current node matches
    if (node.value == target) {
        if (node isClassOf BinarySearchNode) {
            print("Found " + target + " as BinarySearchNode");
        } else if (node isClassOf AVLNode) {
            print("Found " + target + " as AVLNode");
        } else {
            print("Found " + target + " as TreeNode");
        }
        return true;
    }

    // Search left and right
    if (recursiveSearch(node.left, target)) {
        return true;
    }

    return recursiveSearch(node.right, target);
}

bool found5 = recursiveSearch(root, 5);
bool found15 = recursiveSearch(root, 15);
bool found99 = recursiveSearch(root, 99);

print("Search 5 result: " + found5);
print("Search 15 result: " + found15);
print("Search 99 result: " + found99);

// Test 5: Mutually recursive functions with casting
print("Test 5: Mutually recursive with casting");

function processEven(TreeNode node, int count): int {
    if (node == null || count >= 10) {
        return count;
    }

    if (node.value % 2 == 0) {
        if (node isClassOf BinarySearchNode) {
            BinarySearchNode bsn = (BinarySearchNode)node;
            print("Even BSN: " + node.value + " (" + bsn.getColor() + ")");
        } else {
            print("Even node: " + node.value);
        }
        count = count + 1;
    }

    count = processOdd(node.left, count);
    return processOdd(node.right, count);
}

function processOdd(TreeNode node, int count): int {
    if (node == null || count >= 10) {
        return count;
    }

    if (node.value % 2 == 1) {
        if (node isClassOf AVLNode) {
            AVLNode avl = (AVLNode)node;
            print("Odd AVL: " + node.value + " (height: " + avl.getHeight() + ")");
        } else {
            print("Odd node: " + node.value);
        }
        count = count + 1;
    }

    count = processEven(node.left, count);
    return processEven(node.right, count);
}

int processed = processEven(root, 0);
print("Processed count: " + processed);

// Test 6: Tail-recursive style with casting
print("Test 6: Tail-recursive with accumulator");

function tailRecursiveCollect(TreeNode node, int accumulator): int {
    if (node == null) {
        return accumulator;
    }

    // Add current value
    int newAccum = accumulator + node.value;

    // Bonus for red nodes
    if (node isClassOf BinarySearchNode) {
        BinarySearchNode bsn = (BinarySearchNode)node;
        if (bsn.isRed) {
            newAccum = newAccum + 100;
        }
    }

    // Process left
    newAccum = tailRecursiveCollect(node.left, newAccum);

    // Process right (tail position)
    return tailRecursiveCollect(node.right, newAccum);
}

int collected = tailRecursiveCollect(root, 0);
print("Tail recursive collected: " + collected);

print("All recursive casting tests completed");
