// Test multiple interface implementation with same method signature
// Class implements multiple interfaces with identical method signatures
// Should handle casting and method resolution correctly

interface Printable {
    public function display(): string;
}

interface Viewable {
    public function display(): string;
}

interface Showable {
    public function display(): string;
}

class MultiDisplay implements Printable, Viewable, Showable {
    string content;

    constructor(string c) {
        this.content = c;
    }

    public function display(): string {
        return this.content;
    }
}

// Create instance
MultiDisplay obj = new MultiDisplay("shared implementation");

// Cast to each interface
Printable printable = (Printable)obj;
Viewable viewable = (Viewable)obj;
Showable showable = (Showable)obj;

// All should call the same implementation
print(printable.display());
print(viewable.display());
print(showable.display());

// Direct invocation
print(obj.display());

// Chain casting
Printable p = (Printable)obj;
Viewable v = (Viewable)obj;
print(p.display() + " == " + v.display());

print("Multiple interface same method test passed");
