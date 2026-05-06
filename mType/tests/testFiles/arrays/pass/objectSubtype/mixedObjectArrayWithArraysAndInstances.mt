// MYT-281: an Object[] slot accepts both class instances AND arrays in
// the same backing store. Exercises both code paths in the
// ArrayExecutor::setNativeArrayElement Object[] short-circuit.
print("Testing mixed Object[] with arrays and class instances");

class Tag {
    public string name;
    constructor(string n) {
        this.name = n;
    }
}

Object[] mixed = new Object[4];
mixed[0] = new Tag("alpha");
mixed[1] = new int[2];
mixed[2] = new Tag("beta");
mixed[3] = new string[3];

// Round-trip the class instances.
Tag t0 = (Tag)mixed[0];
Tag t2 = (Tag)mixed[2];
print("Tag 0 name: " + t0.name);
print("Tag 2 name: " + t2.name);

// Round-trip the arrays.
int[] arr1 = (int[])mixed[1];
string[] arr3 = (string[])mixed[3];
print("Arr 1 length: " + arr1.length);
print("Arr 3 length: " + arr3.length);

print("Done");
