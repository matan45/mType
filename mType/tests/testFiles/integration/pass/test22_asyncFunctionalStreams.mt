// Test 22: Async, Functional Streams, Recursive
// Features: Async, lambda, Collections with Stream API, recursive functions.

import * from "../../lib/stream/Stream.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

class TreeNode {
    private string name;
    private ArrayList<TreeNode> children;
    
    public constructor(string name) {
        this.name = name;
        this.children = new ArrayList<TreeNode>();
    }
    
    public function getName(): string {
        return this.name;
    }
    
    public function addChild(TreeNode child): void {
        this.children.add(child);
    }
    
    public function getChildren(): ArrayList<TreeNode> {
        return this.children;
    }
}

function async fakeDelayAsync(): Promise<void> {
    // Empty async function to force await points
    return;
}

// Recursive async traversal
function async processNodeAsync(TreeNode node, int depth): Promise<Int> {
    string prefix = "";
    for (int i = 0; i < depth; i = i + 1) {
        prefix = prefix + "  ";
    }
    
    print(prefix + "Processing: " + node.getName());
    
    int count = 1;
    
    ArrayList<TreeNode> kids = node.getChildren();
    if (kids.size() > 0) {
        // Stream API: filter out nodes with "skip"
        Stream<TreeNode> validKidsStream = kids.stream().filter(n -> n.getName() != "skip");
        
        TreeNode[] validKids = validKidsStream.toArray();
        
        for (int i = 0; i < validKids.length; i = i + 1) {
            TreeNode child = validKids[i];
            
            await fakeDelayAsync(); // simulate async work
            
            Int childCount = await processNodeAsync(child, depth + 1);
            count = count + childCount.getValue();
        }
    }
    
    return new Int(count);
}

function async main(): Promise<void> {
    print("--- Test 22: Async Functional Streams ---");
    
    TreeNode root = new TreeNode("root");
    TreeNode c1 = new TreeNode("child1");
    TreeNode c2 = new TreeNode("skip"); // Should be filtered out by stream
    TreeNode c3 = new TreeNode("child3");
    
    TreeNode c1a = new TreeNode("child1a");
    
    c1.addChild(c1a);
    c1.addChild(new TreeNode("skip")); // This one should also be filtered
    
    root.addChild(c1);
    root.addChild(c2);
    root.addChild(c3);
    
    Int totalProcessed = await processNodeAsync(root, 0);
    print("Total nodes processed: " + totalProcessed.getValue());
}

main();