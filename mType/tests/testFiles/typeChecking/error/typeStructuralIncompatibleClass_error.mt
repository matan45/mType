// Tier C (advanced features → no error tests today): mType uses NOMINAL
// typing — two classes with identical structure are not compatible. Verify
// the type checker rejects same-shape but different-name assignments.
// Counterpart to typeStructuralTyping_pass (which uses interfaces).
// Targets: TypeValidator.cpp:191-252 validateObjectTypeAssignment.

class Point2D {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

class Coordinate {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

Point2D p = new Point2D(3, 4);

// Same shape but different class — must error in nominal typing.
Coordinate c = p;

print(c.x);
