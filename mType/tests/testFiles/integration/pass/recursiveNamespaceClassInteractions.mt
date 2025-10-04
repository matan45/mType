// Test recursive patterns with namespace and class interactions

    class Node {
        int value;
        string name;

        public constructor(int val, string nodeName) {
            value = val;
            name = nodeName;
        }

        public function processNode(int depth): int {
            if (depth <= 0) {
                return value;
            }

            // Create child node
            Node child = new Node(value + 1, name + "_child");
            return value + child.processNode(depth - 1);
        }

        public function getValue(): int {
            return value;
        }
    }
    

        function buildTree(int levels): int {
            Node root = new Node(1, "root");
            return root.processNode(levels);
        }
        
        function processMultipleNodes(): int {
            int total = 0;
            
            for (int i = 0; i < 3; i++) {
                Node node = new Node(i * 10, "node" + i);
                total = total + node.processNode(2);
            }
            
            return total;
        }
    


// Test recursive interactions
int treeResult = buildTree(3);
int multiNodeResult = processMultipleNodes();

print(treeResult);
print(multiNodeResult);

// Test direct recursive calls
Node testNode = new Node(5, "test");
int directResult = testNode.processNode(4);
print(directResult);