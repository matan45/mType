// MYT-378: an interface-typed array at SoA size (Drawable[16]) holds concrete
// implementors. An implementor is never the exact interface "class", so every
// store goes through heterogeneous storage; interface-method dispatch must
// survive the fallback.
print("Testing interface array at size 16");

interface Drawable {
    function draw(): string;
}

class Circle implements Drawable {
    public function draw(): string {
        return "Circle";
    }
}

class Square implements Drawable {
    public function draw(): string {
        return "Square";
    }
}

Drawable[] shapes = new Drawable[16];
for (int i = 0; i < 16; i++) {
    if (i < 8) {
        shapes[i] = new Circle();
    } else {
        shapes[i] = new Square();
    }
}

for (int i = 0; i < 16; i++) {
    print(shapes[i].draw());
}

print("Done");

// Expected output:
// Testing interface array at size 16
// Circle
// Circle
// Circle
// Circle
// Circle
// Circle
// Circle
// Circle
// Square
// Square
// Square
// Square
// Square
// Square
// Square
// Square
// Done
