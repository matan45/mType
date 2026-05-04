// MYT-274: when only the child class lacks hashCode/equals, synthesis must
// include inherited instance fields so two child instances compare equal iff
// BOTH parent fields and child fields match.

class Shape {
    private int kind;

    public constructor(int kind) {
        this.kind = kind;
    }
}

class Sized extends Shape {
    private int size;

    public constructor(int kind, int size) : super(kind) {
        this.size = size;
    }
}

Sized a = new Sized(1, 10);
Sized b = new Sized(1, 10);
Sized c = new Sized(1, 20);
Sized d = new Sized(2, 10);

print("equal both fields: " + a.equals(b));
print("differing child field: " + a.equals(c));
print("differing parent field: " + a.equals(d));
print("hash equal both fields: " + (a.hashCode() == b.hashCode()));
