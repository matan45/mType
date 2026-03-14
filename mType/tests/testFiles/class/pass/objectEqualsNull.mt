// Test: equals(null) via native Object fallback
// Expected: Pass - a class with no equals override calls Object's native equals(null) → false

class Simple {
    public int x;

    public constructor(int x) {
        this.x = x;
    }
}

Simple a = new Simple(1);
Simple b = new Simple(1);
Simple c = new Simple(2);

// equals(Simple) — resolved as typed overload against Object::equals(Object)
// Since Simple has no equals override, dispatches to native Object contentEquals
print("a equals b: " + a.equals(b));
print("a equals c: " + a.equals(c));

// equals(null) — null argument, should return false via native fallback
print("a equals null: " + a.equals(null));

// hashCode and toString also via native fallback
int h = a.hashCode();
print("a has hashCode: " + (h != 0));

string s = a.toString();
print("a toString not empty: " + (s != ""));
