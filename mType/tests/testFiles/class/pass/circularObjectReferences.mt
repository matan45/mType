// Test circular object references
// This tests various scenarios where objects reference each other in circular patterns

// Test 1: Simple circular reference (A -> B -> A)
print("Testing simple circular reference:");

class Node {
    string name;
    Node next;
    
    constructor(string n) {
        name = n;
        next = null;
    }
    
    function setNext(Node n): void {
        next = n;
    }
    
    function getNext(): Node {
        return next;
    }
    
    function getName(): string {
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
    string value;
    SelfRef self;
    
    constructor(string v) {
        value = v;
        self = null;
    }
    
    function setSelf(): void {
        self = this;
    }
    
    function getSelf(): SelfRef {
        return self;
    }
    
    function getValue(): string {
        return value;
    }
}

SelfRef selfObj = new SelfRef("SelfReference");
selfObj.setSelf();

print("Object value: " + selfObj.getValue());
print("Self reference value: " + selfObj.getSelf().getValue());
SelfRef temp = selfObj.getSelf();
print("Self->Self value: " + temp.getSelf().getValue());

// Test 3: Complex circular chain (A -> B -> C -> A)
print("\nTesting complex circular chain:");

class ChainNode {
    string id;
    ChainNode link;
    int data;
    
    constructor(string i, int d) {
        id = i;
        data = d;
        link = null;
    }
    
    function setLink(ChainNode n): void {
        link = n;
    }
    
    function getLink(): ChainNode {
        return link;
    }
    
    function getId(): string {
        return id;
    }
    
    function getData(): int {
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
    print("Step " + toString(i) + ": " + current.getId() + " (data=" + toString(current.getData()) + ")");
    current = current.getLink();
}

// Test 4: Parent-child circular reference
print("\nTesting parent-child circular reference:");

class Parent {
    string name;
    Child child;
    
    constructor(string n) {
        name = n;
        child = null;
    }
    
    function setChild(Child c): void {
        child = c;
    }
    
    function getChild(): Child {
        return child;
    }
    
    function getName(): string {
        return name;
    }
}

class Child {
    string name;
    Parent parent;
    
    constructor(string n) {
        name = n;
        parent = null;
    }
    
    function setParent(Parent p): void {
        parent = p;
    }
    
    function getParent(): Parent {
        return parent;
    }
    
    function getName(): string {
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
Child tempChild = parentObj.getChild();
print("Parent -> Child -> Parent: " + tempChild.getParent().getName());
Parent tempParent = childObj.getParent();
print("Child -> Parent -> Child: " + tempParent.getChild().getName());

// Test 5: Doubly linked circular structure
print("\nTesting doubly linked circular structure:");

class DNode {
    string data;
    DNode prev;
    DNode next;
    
    constructor(string d) {
        data = d;
        prev = null;
        next = null;
    }
    
    function setPrev(DNode p): void {
        prev = p;
    }
    
    function setNext(DNode n): void {
        next = n;
    }
    
    function getPrev(): DNode {
        return prev;
    }
    
    function getNext(): DNode {
        return next;
    }
    
    function getData(): string {
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
    string label;
    GraphNode neighbor1;
    GraphNode neighbor2;
    
    constructor(string l) {
        label = l;
        neighbor1 = null;
        neighbor2 = null;
    }
    
    function setNeighbors(GraphNode n1, GraphNode n2): void {
        neighbor1 = n1;
        neighbor2 = n2;
    }
    
    function getNeighbor1(): GraphNode {
        return neighbor1;
    }
    
    function getNeighbor2(): GraphNode {
        return neighbor2;
    }
    
    function getLabel(): string {
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
    string name;
    MultiRef ref1;
    MultiRef ref2;
    MultiRef ref3;
    
    constructor(string n) {
        name = n;
        ref1 = null;
        ref2 = null;
        ref3 = null;
    }
    
    function setRefs(MultiRef r1, MultiRef r2, MultiRef r3): void {
        ref1 = r1;
        ref2 = r2;
        ref3 = r3;
    }
    
    function getName(): string {
        return name;
    }
    
    function getRef1(): MultiRef {
        return ref1;
    }
    
    function getRef2(): MultiRef {
        return ref2;
    }
    
    function getRef3(): MultiRef {
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
    string id;
    IndirectRef partner;
    
    constructor(string i) {
        id = i;
        partner = null;
    }
    
    function setPartner(IndirectRef p): void {
        partner = p;
    }
    
    function getPartner(): IndirectRef {
        return partner;
    }
    
    function getPartnerOfPartner(): IndirectRef {
        if (partner != null) {
            return partner.getPartner();
        }
        return null;
    }
    
    function getId(): string {
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
    string value;
    int counter;
    ModNode next;
    
    constructor(string v, int c) {
        value = v;
        counter = c;
        next = null;
    }
    
    function setNext(ModNode n): void {
        next = n;
    }
    
    function increment(): void {
        counter = counter + 1;
    }
    
    function getValue(): string {
        return value + "(" + toString(counter) + ")";
    }
    
    function getNext(): ModNode {
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
ModNode temp = mod1.getNext();
ModNode next2 = temp.getNext();
next2.increment();

print("After modifications: " + mod1.getValue() + " -> " + mod2.getValue());

print("\nCircular object references test completed successfully");