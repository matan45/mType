// Test mutual dependency between interfaces
// @Script

interface Node {
    func getId(): Int;
    func getEdges(): EdgeList;
}

interface EdgeList {
    func add(edge: Edge): void;
    func getNode(index: Int): Node;
    func size(): Int;
}

interface Edge {
    func getSource(): Node;
    func getTarget(): Node;
}

class GraphNode implements Node {
    var id: Int;
    var edges: EdgeList;

    func init(id: Int) {
        this.id = id;
        this.edges = new SimpleEdgeList();
    }

    func getId(): Int {
        return this.id;
    }

    func getEdges(): EdgeList {
        return this.edges;
    }

    func addEdge(target: Node): void {
        var edge = new GraphEdge(this, target);
        this.edges.add(edge);
    }
}

class SimpleEdgeList implements EdgeList {
    var edges: Array<Edge>;

    func init() {
        this.edges = new Array<Edge>();
    }

    func add(edge: Edge): void {
        this.edges.add(edge);
    }

    func getNode(index: Int): Node {
        var edge = this.edges.get(index);
        return edge.getTarget();
    }

    func size(): Int {
        return this.edges.size();
    }
}

class GraphEdge implements Edge {
    var source: Node;
    var target: Node;

    func init(source: Node, target: Node) {
        this.source = source;
        this.target = target;
    }

    func getSource(): Node {
        return this.source;
    }

    func getTarget(): Node {
        return this.target;
    }
}

// Create a simple graph
var node1 = new GraphNode(1);
var node2 = new GraphNode(2);
var node3 = new GraphNode(3);

node1.addEdge(node2);
node1.addEdge(node3);
node2.addEdge(node3);

print("Node 1 has " + node1.getEdges().size().toString() + " edges");
print("Node 2 has " + node2.getEdges().size().toString() + " edges");

var edges = node1.getEdges();
var i = 0;
while (i < edges.size()) {
    var targetNode = edges.getNode(i);
    print("Node 1 connects to node " + targetNode.getId().toString());
    i = i + 1;
}
