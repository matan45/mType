// Combo 14: Reflection + Value Class + Annotation
// Tests: reflecting on value class fields/methods + annotation metadata

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Constructor.mt";

@Retention(RUNTIME)
@Target([FIELD])
annotation Unit {
    string name = "";
    string symbol = "";
}

@Retention(RUNTIME)
@Target([METHOD])
annotation Pure {}

@Retention(RUNTIME)
@Target([CLASS])
annotation Immutable {}

@Immutable
value class Vector2D {
    @Unit(name = "pixels", symbol = "px")
    private float x;

    @Unit(name = "pixels", symbol = "px")
    private float y;

    public constructor(float x, float y) {
        this.x = x;
        this.y = y;
    }

    public function getX(): float { return this.x; }
    public function getY(): float { return this.y; }

    @Pure
    public function magnitude(): float {
        return (this.x * this.x + this.y * this.y);
    }

    @Pure
    public function add(Vector2D other): Vector2D {
        return new Vector2D(this.x + other.getX(), this.y + other.getY());
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

function main(): void {
    print("=== Combo 14: Reflection + Value Class + Annotation ===");

    // Use the value class normally
    print("--- Value class usage ---");
    Vector2D v1 = new Vector2D(3.0, 4.0);
    Vector2D v2 = new Vector2D(1.0, 2.0);
    Vector2D v3 = v1.add(v2);
    print("v1: " + v1.toString());
    print("v2: " + v2.toString());
    print("v1 + v2: " + v3.toString());
    print("v1 magnitude^2: " + v1.magnitude());

    // Reflect on value class
    print("--- Reflection on value class ---");
    Class cls = Class::forName("Vector2D");
    print("Class name: " + cls.getName());

    // Check class-level annotation
    bool isImmutable = cls.hasAnnotation("Immutable");
    print("Has @Immutable: " + isImmutable);

    // Inspect fields + annotations
    print("--- Field annotations ---");
    Field[] fields = cls.getFields();
    print("Field count: " + fields.length);
    for (int i = 0; i < fields.length; i++) {
        Field f = fields[i];
        print("Field: " + f.getName());
        if (f.hasAnnotation("Unit")) {
            Annotation? unit = f.getAnnotation("Unit");
            if (unit != null) {
                print("  Unit: " + unit.getString("name") + " (" + unit.getString("symbol") + ")");
            }
        }
    }

    // Inspect methods + @Pure annotation
    print("--- Method annotations ---");
    Method[] methods = cls.getDeclaredMethods();
    for (int i = 0; i < methods.length; i++) {
        Method m = methods[i];
        if (m.hasAnnotation("Pure")) {
            print("@Pure method: " + m.getName());
        }
    }

    // Constructor info
    print("--- Constructor info ---");
    Constructor[] ctors = cls.getConstructors();
    print("Constructor count: " + ctors.length);

    print("=== Combo 14 Complete ===");
}

main();
