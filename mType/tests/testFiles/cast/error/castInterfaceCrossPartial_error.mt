// Test cross-cast with partial interface implementation
// Class implements Interface1 but not Interface2
// Attempting to cast from Interface1 to Interface2 should fail
// @Throw

interface Drawable {
    function draw(): void;
}

interface Serializable {
    function serialize(): string;
}

interface Printable {
    function print(): void;
}

// Class only implements Drawable
class Shape implements Drawable {
    string name;

    constructor(string n) {
        this.name = n;
    }

    public function draw(): void {
        print("Drawing " + this.name);
    }
}

// Create instance and cast to Drawable
Shape shape = new Shape("circle");
Drawable drawable = (Drawable)shape;

print("Cast to Drawable successful");

// ERROR: Attempt to cross-cast to unimplemented interface
// Shape does not implement Serializable
Serializable serializable = (Serializable)drawable;
print(serializable.serialize());
