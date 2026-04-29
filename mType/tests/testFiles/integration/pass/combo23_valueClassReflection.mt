// Combo 23: Value Class + Reflection on Fields
// Tests: enumerate value class fields via reflection; instantiate; read values

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Constructor.mt";

value class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int { return this.x; }
    public function getY(): int { return this.y; }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

function main(): void {
    print("=== Combo 23: Value Class Reflection ===");

    print("--- Class info ---");
    Class cls = Class::forName("Point");
    print("Class: " + cls.getName());

    print("--- Fields ---");
    Field[] fields = cls.getDeclaredFields();
    print("Field count: " + fields.length);
    for (int i = 0; i < fields.length; i++) {
        Field f = fields[i];
        print("Field " + i + ": " + f.getName() + " type=" + f.getType());
    }

    print("--- Constructors ---");
    Constructor[] ctors = cls.getConstructors();
    print("Constructor count: " + ctors.length);

    print("--- Instantiate + read ---");
    Point p1 = new Point(3, 7);
    print("p1 = " + p1.toString());
    print("p1.x = " + p1.getX());
    print("p1.y = " + p1.getY());

    Point p2 = new Point(10, 20);
    print("p2 = " + p2.toString());
    print("p2.x = " + p2.getX());
    print("p2.y = " + p2.getY());

    print("=== Combo 23 Complete ===");
}

main();
