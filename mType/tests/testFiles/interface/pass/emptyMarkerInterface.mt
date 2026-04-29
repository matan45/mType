// Empty marker interface - no methods. Used purely for type tagging.
interface Marker {}

class Tagged implements Marker {
    public int id;
    constructor(int i) {
        this.id = i;
    }
}

class AlsoTagged implements Marker {
    public string label;
    constructor(string l) {
        this.label = l;
    }
}

class Untagged {
    public int x;
    constructor(int v) {
        this.x = v;
    }
}

Tagged t = new Tagged(1);
AlsoTagged a = new AlsoTagged("hello");
Untagged u = new Untagged(42);

print("t isClassOf Marker: " + (t isClassOf Marker));
print("a isClassOf Marker: " + (a isClassOf Marker));
print("u isClassOf Marker: " + (u isClassOf Marker));
