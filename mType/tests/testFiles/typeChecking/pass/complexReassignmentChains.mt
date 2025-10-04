class DataContainer {
    public string value;
    constructor(string v) {
        value = v;
    }

    public function setValue(string v) : void{
        value = v;
    }
}

function createContainer(string val): DataContainer {
    return new DataContainer(val);
}

// Test reassignment through function returns
DataContainer d1 = new DataContainer("Initial");
DataContainer d2 = new DataContainer("Second");
DataContainer d3 = new DataContainer("Third");

// Chain with function call
d1 = d2 = createContainer("Function");
print("Function chain result: " + d1.value);

// Chain with direct assignments
DataContainer source = new DataContainer("Source");
d1 = d2 = d3 = source;
print("Chain result: " + d1.value);
print("Chain result 2: " + d2.value);
print("Chain result 3: " + d3.value);

// Complex pattern with intermediate modifications
DataContainer x = new DataContainer("X");
DataContainer y = new DataContainer("Y");
DataContainer z = new DataContainer("Z");

x = y;
y.setValue("Modified");
z = x;
print("Complex modification chain X: " + x.value);
print("Complex modification chain Y: " + y.value);
print("Complex modification chain Z: " + z.value);

// Test reassignment with null propagation
DataContainer n1 = new DataContainer("N1");
DataContainer n2 = null;
DataContainer n3 = new DataContainer("N3");

n1 = n2;  // n1 becomes null
n3 = n1;  // n3 becomes null
if (n3 == null) {
    print("Null propagation successful");
}

// Test nested object reassignment chains
class NestedContainer {
    public DataContainer inner;
    constructor(string val) {
        inner = new DataContainer(val);
    }
}

NestedContainer nc1 = new NestedContainer("Nested1");
NestedContainer nc2 = new NestedContainer("Nested2");
NestedContainer nc3 = new NestedContainer("Nested3");

nc1.inner = nc2.inner = nc3.inner;
print("Nested reassignment: " + nc1.inner.value);
print("Nested reassignment 2: " + nc2.inner.value);