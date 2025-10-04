// Test circular object references
// This tests various scenarios where objects reference each other in circular patterns

// Test 1: Simple circular reference (A -> B -> A)
print("Testing simple circular reference:");

class Node {
    public string name;
    public Node next;

    public constructor(string n) {
        name = n;
        next = null;
    }

    public function setNext(Node n): void {
        next = n;
    }

    public function getNext(): Node {
        return next;
    }

    public function getName(): string {
        return name;
    }
}

Node nodeA = new Node("A");
Node nodeB = new Node("B");

// Create circular reference
nodeA.setNext(nodeB);
nodeB.setNext(nodeA);

print("NodeA -> " + nodeA.getNext().getName());
print("NodeB -> " + nodeB.getNext().getName());
Node tempA = nodeA.getNext();
print("NodeA -> NodeB -> " + tempA.getNext().getName());
Node tempB = nodeB.getNext();
print("NodeB -> NodeA -> " + tempB.getNext().getName());

// Test 2: Self-reference
print("\nTesting self-reference:");

class SelfRef {
    public string value;
    public SelfRef self;

    public constructor(string v) {
        value = v;
        self = null;
    }

    public function setSelf(): void {
        self = this;
    }

    public function getSelf(): SelfRef {
        return self;
    }

    public function getValue(): string {
        return value;
    }
}

SelfRef selfObj = new SelfRef("SelfReference");
selfObj.setSelf();

print("Object value: " + selfObj.getValue());
print("Self reference value: " + selfObj.getSelf().getValue());
SelfRef tempSelf = selfObj.getSelf();
print("Self->Self value: " + tempSelf.getSelf().getValue());

// Test 3: Complex circular chain (A -> B -> C -> A)
print("\nTesting complex circular chain:");

class ChainNode {
    public string id;
    public ChainNode link;
    public int data;

    public constructor(string i, int d) {
        id = i;
        data = d;
        link = null;
    }

    public function setLink(ChainNode n): void {
        link = n;
    }

    public function getLink(): ChainNode {
        return link;
    }

    public function getId(): string {
        return id;
    }

    public function getData(): int {
        return data;
    }
}

ChainNode node1 = new ChainNode("First", 1);
ChainNode node2 = new ChainNode("Second", 2);
ChainNode node3 = new ChainNode("Third", 3);

// Create circular chain
node1.setLink(node2);
node2.setLink(node3);
node3.setLink(node1);

// Traverse the circular chain
print("Chain traversal:");
print(node1.getId() + " -> " + node1.getLink().getId());
print(node2.getId() + " -> " + node2.getLink().getId());
print(node3.getId() + " -> " + node3.getLink().getId());

// Full circle traversal
ChainNode current = node1;
for (int i = 0; i < 6; i++) {
    print("Step " + i + ": " + current.getId() + " (data=" + current.getData() + ")");
    current = current.getLink();
}

// Test 4: Parent-child circular reference
print("\nTesting parent-child circular reference:");

class Parent {
    public string name;
    public Child child;

    public constructor(string n) {
        name = n;
        child = null;
    }

    public function setChild(Child c): void {
        child = c;
    }

    public function getChild(): Child {
        return child;
    }

    public function getName(): string {
        return name;
    }
}

class Child {
    public string name;
    public Parent parent;

    public constructor(string n) {
        name = n;
        parent = null;
    }

    public function setParent(Parent p): void {
        parent = p;
    }

    public function getParent(): Parent {
        return parent;
    }

    public function getName(): string {
        return name;
    }
}

Parent parentObj = new Parent("ParentObject");
Child childObj = new Child("ChildObject");

// Create circular reference between parent and child
parentObj.setChild(childObj);
childObj.setParent(parentObj);

print("Parent -> Child: " + parentObj.getChild().getName());
print("Child -> Parent: " + childObj.getParent().getName());
Child tempChild2 = parentObj.getChild();
print("Parent -> Child -> Parent: " + tempChild2.getParent().getName());
Parent tempParent2 = childObj.getParent();
print("Child -> Parent -> Child: " + tempParent2.getChild().getName());

// Test 5: Doubly linked circular structure
print("\nTesting doubly linked circular structure:");

class DNode {
    public string data;
    public DNode prev;
    public DNode next;

    public constructor(string d) {
        data = d;
        prev = null;
        next = null;
    }

    public function setPrev(DNode p): void {
        prev = p;
    }

    public function setNext(DNode n): void {
        next = n;
    }

    public function getPrev(): DNode {
        return prev;
    }

    public function getNext(): DNode {
        return next;
    }

    public function getData(): string {
        return data;
    }
}

DNode dnode1 = new DNode("Node1");
DNode dnode2 = new DNode("Node2");
DNode dnode3 = new DNode("Node3");

// Create doubly linked circular list
dnode1.setNext(dnode2);
dnode1.setPrev(dnode3);

dnode2.setNext(dnode3);
dnode2.setPrev(dnode1);

dnode3.setNext(dnode1);
dnode3.setPrev(dnode2);

print("Doubly linked circular traversal:");
print(dnode1.getData() + " -> next: " + dnode1.getNext().getData() + ", prev: " + dnode1.getPrev().getData());
print(dnode2.getData() + " -> next: " + dnode2.getNext().getData() + ", prev: " + dnode2.getPrev().getData());
print(dnode3.getData() + " -> next: " + dnode3.getNext().getData() + ", prev: " + dnode3.getPrev().getData());

// Test 6: Graph-like circular references
print("\nTesting graph-like circular references:");

class GraphNode {
    public string label;
    public GraphNode neighbor1;
    public GraphNode neighbor2;

    public constructor(string l) {
        label = l;
        neighbor1 = null;
        neighbor2 = null;
    }

    public function setNeighbors(GraphNode n1, GraphNode n2): void {
        neighbor1 = n1;
        neighbor2 = n2;
    }

    public function getNeighbor1(): GraphNode {
        return neighbor1;
    }

    public function getNeighbor2(): GraphNode {
        return neighbor2;
    }

    public function getLabel(): string {
        return label;
    }
}

GraphNode gnode1 = new GraphNode("G1");
GraphNode gnode2 = new GraphNode("G2");
GraphNode gnode3 = new GraphNode("G3");

// Create circular graph structure
gnode1.setNeighbors(gnode2, gnode3);
gnode2.setNeighbors(gnode3, gnode1);
gnode3.setNeighbors(gnode1, gnode2);

print("Graph structure:");
print(gnode1.getLabel() + " -> " + gnode1.getNeighbor1().getLabel() + ", " + gnode1.getNeighbor2().getLabel());
print(gnode2.getLabel() + " -> " + gnode2.getNeighbor1().getLabel() + ", " + gnode2.getNeighbor2().getLabel());
print(gnode3.getLabel() + " -> " + gnode3.getNeighbor1().getLabel() + ", " + gnode3.getNeighbor2().getLabel());

// Test 7: Circular reference with null breaking
print("\nTesting circular reference with null breaking:");

Node breakNode1 = new Node("Break1");
Node breakNode2 = new Node("Break2");

breakNode1.setNext(breakNode2);
breakNode2.setNext(breakNode1);

Node bn1Next = breakNode1.getNext();
print("Before breaking: " + bn1Next.getName() + " -> " + bn1Next.getNext().getName());

// Break the circular reference
breakNode2.setNext(null);

Node afterBreak = breakNode1.getNext();
print("After breaking: " + afterBreak.getName());
if (afterBreak.getNext() == null) {
    print("Circular reference successfully broken");
}

// Test 8: Multiple circular references in same object
print("\nTesting multiple circular references:");

class MultiRef {
    public string name;
    public MultiRef ref1;
    public MultiRef ref2;
    public MultiRef ref3;

    public constructor(string n) {
        name = n;
        ref1 = null;
        ref2 = null;
        ref3 = null;
    }

    public function setRefs(MultiRef r1, MultiRef r2, MultiRef r3): void {
        ref1 = r1;
        ref2 = r2;
        ref3 = r3;
    }

    public function getName(): string {
        return name;
    }

    public function getRef1(): MultiRef {
        return ref1;
    }

    public function getRef2(): MultiRef {
        return ref2;
    }

    public function getRef3(): MultiRef {
        return ref3;
    }
}

MultiRef multi1 = new MultiRef("Multi1");
MultiRef multi2 = new MultiRef("Multi2");

// Create multiple circular paths
multi1.setRefs(multi2, multi1, multi2);
multi2.setRefs(multi1, multi2, multi1);

print("Multi1 refs: " + multi1.getRef1().getName() + ", " + multi1.getRef2().getName() + ", " + multi1.getRef3().getName());
print("Multi2 refs: " + multi2.getRef1().getName() + ", " + multi2.getRef2().getName() + ", " + multi2.getRef3().getName());

// Test 9: Indirect circular reference through method returns
print("\nTesting indirect circular references:");

class IndirectRef {
    public string id;
    public IndirectRef partner;

    public constructor(string i) {
        id = i;
        partner = null;
    }

    public function setPartner(IndirectRef p): void {
        partner = p;
    }

    public function getPartner(): IndirectRef {
        return partner;
    }

    public function getPartnerOfPartner(): IndirectRef {
        if (partner != null) {
            return partner.getPartner();
        }
        return null;
    }

    public function getId(): string {
        return id;
    }
}

IndirectRef ind1 = new IndirectRef("Indirect1");
IndirectRef ind2 = new IndirectRef("Indirect2");

ind1.setPartner(ind2);
ind2.setPartner(ind1);

print("Indirect reference: " + ind1.getId() + " -> partner's partner: " + ind1.getPartnerOfPartner().getId());

// Test 10: Circular reference with modification
print("\nTesting circular reference with modification:");

class ModNode {
    public string value;
    public int counter;
    public ModNode next;

    public constructor(string v, int c) {
        value = v;
        counter = c;
        next = null;
    }

    public function setNext(ModNode n): void {
        next = n;
    }

    public function increment(): void {
        counter = counter + 1;
    }

    public function getValue(): string {
        return value + "(" + counter + ")";
    }

    public function getNext(): ModNode {
        return next;
    }
}

ModNode mod1 = new ModNode("Mod1", 0);
ModNode mod2 = new ModNode("Mod2", 0);

mod1.setNext(mod2);
mod2.setNext(mod1);

print("Initial state: " + mod1.getValue() + " -> " + mod2.getValue());

// Modify through circular reference
mod1.increment();
ModNode next1 = mod1.getNext();
next1.increment();
ModNode tempMod = mod1.getNext();
ModNode next2 = tempMod.getNext();
next2.increment();

print("After modifications: " + mod1.getValue() + " -> " + mod2.getValue());

print("\nCircular object references test completed successfully");