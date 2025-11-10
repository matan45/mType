// Test: Ambiguous method overload resolution
// Expected: Error - cannot resolve which overload to call

class AmbiguousClass {
    // These two methods create ambiguity with null arguments
    public void process(string s) {
        print("process(string): " + s);
    }

    public void process(Object o) {
        print("process(Object): " + o);
    }
}

AmbiguousClass obj = new AmbiguousClass();
// This call is ambiguous - null could be string or Object
obj.process(null);
