// Test casting between marker interfaces
// Marker interfaces have no methods - used for type marking

interface Serializable {
    // Marker interface - no methods
}

interface Cloneable {
    // Marker interface - no methods
}

interface Persistable extends Serializable {
    // Extends marker interface
}

class DataObject implements Serializable, Cloneable {
    string data;

    constructor(string d) {
        this.data = d;
    }

    public function getData(): string {
        return this.data;
    }
}

class PersistentData implements Persistable, Cloneable {
    string data;

    constructor(string d) {
        this.data = d;
    }

    public function getData(): string {
        return this.data;
    }
}

// Test casting to marker interfaces
DataObject obj1 = new DataObject("test data");
Serializable ser = (Serializable)obj1;
Cloneable clone = (Cloneable)obj1;

print("Marker interface cast 1 successful");

// Test with inheritance of marker interface
PersistentData obj2 = new PersistentData("persistent data");
Persistable per = (Persistable)obj2;
Serializable ser2 = (Serializable)per;  // Cast to parent marker
Cloneable clone2 = (Cloneable)obj2;

print("Marker interface cast 2 successful");

// Verify object functionality still works after casting
DataObject obj3 = new DataObject("working");
Serializable temp = (Serializable)obj3;
DataObject back = (DataObject)temp;
print(back.getData());

print("Marker interface casting test passed");
