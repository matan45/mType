// Test mutual dependency between interfaces
// @Script

import * from "../../lib/collections/ArrayList.mt";

interface Node {
    function getId(): int;
    function getEdges(): EdgeArrayList;
}

interface EdgeArrayList {
    function add(Edge edge): void;
    function getNode(int index): Node;
    function size(): int;
}

interface Edge {
    function getSource(): Node;
    function getTarget(): Node;
}

class GraphNode implements Node {
    private int id;
    private EdgeArrayList edges;

    public constructor(int id) {
        this.id = id;
        this.edges = new SimpleEdgeArrayList();
    }

    public function getId(): int {
        return this.id;
    }

    public function getEdges(): EdgeArrayList {
        return this.edges;
    }

    public function addEdge(Node target): void {
        GraphEdge edge = new GraphEdge(this, target);
        this.edges.add(edge);
    }
}

class SimpleEdgeArrayList implements EdgeArrayList {
    private ArrayList<Edge> edges;

    public constructor() {
        this.edges = new ArrayList<Edge>();
    }

    public function add(Edge edge): void {
        this.edges.add(edge);
    }

    public function getNode(int index): Node {
        Edge edge = this.edges.get(index);
        return edge.getTarget();
    }

    public function size(): int {
        return this.edges.size();
    }
}

class GraphEdge implements Edge {
    private Node source;
    private Node target;

    public constructor(Node source, Node target) {
        this.source = source;
        this.target = target;
    }

    public function getSource(): Node {
        return this.source;
    }

    public function getTarget(): Node {
        return this.target;
    }
}

// Create a simple graph
GraphNode node1 = new GraphNode(1);
GraphNode node2 = new GraphNode(2);
GraphNode node3 = new GraphNode(3);

node1.addEdge(node2);
node1.addEdge(node3);
node2.addEdge(node3);

print("Node 1 has " + node1.getEdges().size() + " edges");
print("Node 2 has " + node2.getEdges().size() + " edges");

EdgeArrayList edges = node1.getEdges();
int i = 0;
while (i < edges.size()) {
    Node targetNode = edges.getNode(i);
    print("Node 1 connects to node " + targetNode.getId());
    i = i + 1;
}
