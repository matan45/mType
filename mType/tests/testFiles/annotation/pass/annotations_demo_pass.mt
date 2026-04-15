// MYT-108 + MYT-109 (3a) — end-to-end runtime demo of the annotation system.
//
// Walks the full pipeline in one file:
//   1. Annotation declaration with typed parameters (MYT-108).
//   2. Meta-annotated annotation declarations using @Retention / @Target.
//   3. Constructor-, method-, and field-level annotation usage.
//   4. Reflection: typed parameter readback.
//   5. Reflection: meta-annotation lookup via Annotation.getMeta.
//   6. @Retention(SOURCE) stripping at class registration.
//
// Doubles as a regression test — output is verified against
// annotations_demo_pass.expected.

import * from "../../lib/reflect/Class.mt";

// ----- 1 + 2. Annotation declarations with meta-annotations -----

@Retention(RUNTIME)
@Target([METHOD])
annotation Logged {
    string level = "info";
}

@Retention(RUNTIME)
@Target([FIELD])
annotation Column {
    string name;
    bool nullable = false;
}

@Retention(RUNTIME)
@Target([CONSTRUCTOR])
annotation Inject { }

// SOURCE retention — visible to the source/AST but stripped before bytecode
// emission, so reflection at runtime cannot see it.
@Retention(SOURCE)
annotation Draft { }

// ----- 3. Apply annotations across the class surface -----

@Draft
class UserService {
    @Column(name = "user_id")
    public int id;

    @Column(name = "email", nullable = true)
    public string email;

    @Inject
    public constructor() {
        this.id = 0;
        this.email = "";
    }

    @Logged(level = "debug")
    public function ping(): void {
    }
}

// ----- 4. Reflection round-trip -----

Class c = Class::forName("UserService");

print("[class]");
print(c.hasAnnotation("Draft"));   // false — SOURCE-stripped
print(c.hasAnnotation("Logged"));  // false — never applied to the class

print("[method ping]");
Method ping = c.getDeclaredMethod("ping", 0);
Annotation? logged = ping.getAnnotation("Logged");
if (logged != null) {
    print(logged.getString("level"));      // debug

    // ----- 5. Meta-annotation lookup -----
    Annotation? retention = logged.getMeta("Retention");
    print(retention != null);              // true
    Annotation? target = logged.getMeta("Target");
    print(target != null);                 // true
}

print("[field id]");
Field idField = c.getField("id");
Annotation? col = idField.getAnnotation("Column");
if (col != null) {
    print(col.getString("name"));          // user_id
    print(col.getBool("nullable"));        // false (default)
}

print("[field email]");
Field emailField = c.getField("email");
Annotation? colEmail = emailField.getAnnotation("Column");
if (colEmail != null) {
    print(colEmail.getString("name"));     // email
    print(colEmail.getBool("nullable"));   // true (explicit)
}

print("[constructor]");
Constructor ctor = c.getConstructors()[0];
print(ctor.hasAnnotation("Inject"));       // true
