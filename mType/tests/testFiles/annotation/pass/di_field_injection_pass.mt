// MYT-376 showcase: Spring-Boot-style dependency injection on data members,
// driven entirely by runtime reflection (no engine support needed). The
// @Qualifier annotation carries a MYT-376 compile-time-folded constant
// (Beans::PRIMARY) which the container reads back via reflection to pick a bean.
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Annotation.mt";

// --- DI annotations -------------------------------------------------------
annotation Inject { }

annotation Qualifier {
    string name;
}

// --- compile-time constants used as annotation metadata (MYT-376) ---------
class Beans {
    public static final string PRIMARY = "primaryRepo";
}

// --- beans ----------------------------------------------------------------
class Repo {
    private string _id;
    public constructor(string id) {
        this._id = id;
    }
    public function name(): string {
        return this._id;
    }
}

// --- a component with injected data members -------------------------------
class Service {
    @Inject
    public Repo repo;

    @Inject
    @Qualifier(name = Beans::PRIMARY)
    public Repo primary;

    public function run(): string {
        return this.repo.name() + "," + this.primary.name();
    }
}

// --- a minimal reflection-driven DI container -----------------------------
class Container {
    // A generic bean registry keyed by string. The bean store is Object[] so a
    // container can hold any component type — the natural, polymorphic form.
    //
    // CANARY (MYT-378): mType currently rejects storing a subtype (Repo) into an
    // Object[] element with "ObjectInstance class mismatch" because the array
    // element-store check compares exact class names instead of allowing
    // subtypes. This test fails until MYT-378 is fixed; do not "fix" it by
    // narrowing the array type — that would hide the bug.
    private string[] keys;
    private Object[] beans;
    private int count;

    public constructor() {
        this.keys = new string[16];
        this.beans = new Object[16];
        this.count = 0;
    }

    public function register(string key, Object bean): void {
        this.keys[this.count] = key;
        this.beans[this.count] = bean;
        this.count = this.count + 1;
    }

    // Field (data-member) injection: for every @Inject field, resolve a bean by
    // its declared type — or, when @Qualifier is present, by the qualifier name
    // (a value produced at compile time by MYT-376 constant folding).
    public function inject(Object target, string className): void {
        Class c = Class::forName(className);
        Field[] fields = c.getDeclaredFields();
        for (int i = 0; i < fields.length; i = i + 1) {
            Field f = fields[i];
            if (f.hasAnnotation("Inject")) {
                string key = f.getType();
                Annotation? q = f.getAnnotation("Qualifier");
                if (q != null) {
                    key = q.getString("name");
                }
                for (int j = 0; j < this.count; j = j + 1) {
                    if (this.keys[j] == key) {
                        f.set(target, this.beans[j]);
                    }
                }
            }
        }
    }
}

// --- wire it up -----------------------------------------------------------
Container ct = new Container();
ct.register("Repo", new Repo("default"));
// The qualifier key matches Beans::PRIMARY, which MYT-376 folds to "primaryRepo"
// inside @Qualifier above; the container reads that folded value back via
// reflection to select this bean. (A literal is used here only because passing
// a `Class::FIELD` static-final access into a typed string parameter is a
// separate mType type-inference gap, independent of annotation folding.)
ct.register("primaryRepo", new Repo("primary"));

Service s = new Service();
ct.inject(s, "Service");
print(s.run());
