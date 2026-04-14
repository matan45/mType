// MYT-108 §7a case 14: annotation applied to a field, reflected via
// Field.getAnnotation. Exercises instance + static field annotation pathways.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Column {
    string name;
}

class Record {
    @Column(name = "id")
    public int id;

    @Column(name = "created_at")
    public static int createdAt;
}

Class c = Class::forName("Record");
Field f1 = c.getDeclaredField("id");
Field f2 = c.getDeclaredField("createdAt");
Annotation? a1 = f1.getAnnotation("Column");
Annotation? a2 = f2.getAnnotation("Column");
if (a1 != null) {
    print(a1.getString("name"));
}
if (a2 != null) {
    print(a2.getString("name"));
}
