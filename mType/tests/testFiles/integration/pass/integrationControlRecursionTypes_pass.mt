// Test: Type checking in recursive functions
// Tests type safety and inference in recursive control flow

// Test 1: Simple recursive type checking
print("Test 1: Simple recursive type checking");

function factorial(int n): int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

print("factorial(5) = " + factorial(5));
print("factorial(7) = " + factorial(7));

// Test 2: Recursive type inference with multiple parameters
print("\nTest 2: Recursive with multiple parameters");

function gcd(int a, int b): int {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

print("gcd(48, 18) = " + gcd(48, 18));
print("gcd(100, 35) = " + gcd(100, 35));

// Test 3: Recursive function with string types
print("\nTest 3: Recursive with string types");

function reverse(string s, int len): string {
    if (len <= 0) {
        return "";
    }
    if (len == 1) {
        return s;
    }
    // Simplified string reverse using character manipulation
    return s + reverse("", len - 1);
}

function repeat(string s, int times): string {
    if (times <= 0) {
        return "";
    }
    if (times == 1) {
        return s;
    }
    return s + repeat(s, times - 1);
}

print("repeat('A', 5) = " + repeat("A", 5));
print("repeat('Hi', 3) = " + repeat("Hi", 3));

// Test 4: Recursive type checking with classes
print("\nTest 4: Recursive with class types");

class Node {
    private int value;
    private Node next;
    private bool hasNext;

    public constructor(int val) {
        this.value = val;
        this.hasNext = false;
    }

    public void setNext(Node n) {
        this.next = n;
        this.hasNext = true;
    }

    public Node getNext(): Node {
        return this.next;
    }

    public bool hasNextNode(): bool {
        return this.hasNext;
    }

    public int getValue(): int {
        return this.value;
    }
}

class LinkedList {
    private Node head;
    private bool hasHead;

    public constructor() {
        this.hasHead = false;
    }

    public void add(int val) {
        Node newNode = new Node(val);
        if (!this.hasHead) {
            this.head = newNode;
            this.hasHead = true;
        } else {
            Node current = this.head;
            while (current.hasNextNode()) {
                current = current.getNext();
            }
            current.setNext(newNode);
        }
    }

    public int sumRecursive(Node node, bool hasNode): int {
        if (!hasNode) {
            return 0;
        }
        int currentValue = node.getValue();
        if (!node.hasNextNode()) {
            return currentValue;
        }
        return currentValue + this.sumRecursive(node.getNext(), true);
    }

    public int sum(): int {
        return this.sumRecursive(this.head, this.hasHead);
    }

    public int countRecursive(Node node, bool hasNode): int {
        if (!hasNode) {
            return 0;
        }
        if (!node.hasNextNode()) {
            return 1;
        }
        return 1 + this.countRecursive(node.getNext(), true);
    }

    public int count(): int {
        return this.countRecursive(this.head, this.hasHead);
    }
}

LinkedList list = new LinkedList();
list.add(10);
list.add(20);
list.add(30);
list.add(40);

print("List sum: " + list.sum());
print("List count: " + list.count());

// Test 5: Mutual recursion with type checking
print("\nTest 5: Mutual recursion");

function isEven(int n): bool {
    if (n == 0) {
        return true;
    }
    return isOdd(n - 1);
}

function isOdd(int n): bool {
    if (n == 0) {
        return false;
    }
    return isEven(n - 1);
}

print("isEven(4): " + isEven(4));
print("isOdd(4): " + isOdd(4));
print("isEven(7): " + isEven(7));
print("isOdd(7): " + isOdd(7));

// Test 6: Recursive type checking with arrays
print("\nTest 6: Recursive with arrays");

function sumArrayRecursive(int[] arr, int index, int size): int {
    if (index >= size) {
        return 0;
    }
    return arr[index] + sumArrayRecursive(arr, index + 1, size);
}

function maxArrayRecursive(int[] arr, int index, int size, int currentMax): int {
    if (index >= size) {
        return currentMax;
    }
    int current = arr[index];
    int newMax = currentMax;
    if (current > currentMax) {
        newMax = current;
    }
    return maxArrayRecursive(arr, index + 1, size, newMax);
}

int[] numbers = new int[5];
numbers[0] = 15;
numbers[1] = 42;
numbers[2] = 8;
numbers[3] = 23;
numbers[4] = 16;

print("Array sum: " + sumArrayRecursive(numbers, 0, 5));
print("Array max: " + maxArrayRecursive(numbers, 0, 5, numbers[0]));

// Test 7: Recursive type checking with conditionals
print("\nTest 7: Recursive with complex conditionals");

class TreeNode {
    private int value;
    private TreeNode left;
    private TreeNode right;
    private bool hasLeft;
    private bool hasRight;

    public constructor(int val) {
        this.value = val;
        this.hasLeft = false;
        this.hasRight = false;
    }

    public void setLeft(TreeNode node) {
        this.left = node;
        this.hasLeft = true;
    }

    public void setRight(TreeNode node) {
        this.right = node;
        this.hasRight = true;
    }

    public int getValue(): int {
        return this.value;
    }

    public TreeNode getLeft(): TreeNode {
        return this.left;
    }

    public TreeNode getRight(): TreeNode {
        return this.right;
    }

    public bool hasLeftChild(): bool {
        return this.hasLeft;
    }

    public bool hasRightChild(): bool {
        return this.hasRight;
    }
}

class BinaryTree {
    private TreeNode root;
    private bool hasRoot;

    public constructor(int rootValue) {
        this.root = new TreeNode(rootValue);
        this.hasRoot = true;
    }

    public TreeNode getRoot(): TreeNode {
        return this.root;
    }

    public int sumRecursive(TreeNode node, bool hasNode): int {
        if (!hasNode) {
            return 0;
        }
        int sum = node.getValue();
        if (node.hasLeftChild()) {
            sum = sum + this.sumRecursive(node.getLeft(), true);
        }
        if (node.hasRightChild()) {
            sum = sum + this.sumRecursive(node.getRight(), true);
        }
        return sum;
    }

    public int sum(): int {
        return this.sumRecursive(this.root, this.hasRoot);
    }

    public int countRecursive(TreeNode node, bool hasNode): int {
        if (!hasNode) {
            return 0;
        }
        int count = 1;
        if (node.hasLeftChild()) {
            count = count + this.countRecursive(node.getLeft(), true);
        }
        if (node.hasRightChild()) {
            count = count + this.countRecursive(node.getRight(), true);
        }
        return count;
    }

    public int count(): int {
        return this.countRecursive(this.root, this.hasRoot);
    }
}

BinaryTree tree = new BinaryTree(10);
TreeNode root = tree.getRoot();
root.setLeft(new TreeNode(5));
root.setRight(new TreeNode(15));
root.getLeft().setLeft(new TreeNode(3));
root.getLeft().setRight(new TreeNode(7));

print("Tree sum: " + tree.sum());
print("Tree count: " + tree.count());

// Test 8: Fibonacci with type inference
print("\nTest 8: Fibonacci sequence");

function fib(int n): int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

print("fib(0) = " + fib(0));
print("fib(1) = " + fib(1));
print("fib(6) = " + fib(6));
print("fib(8) = " + fib(8));

print("\nRecursion type checking tests completed");
