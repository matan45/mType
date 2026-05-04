// Test: equals/hashCode/toString on a class without explicit overrides.
// MYT-274: synthesis emits equals(Object) non-nullable. A literal `null`
// is accepted at the call site (Object is the universal upper bound)
// and the runtime-null path is handled by the synthesized body's
// `isClassOf` guard — `null isClassOf Simple` is false, so equals
// returns false without ever dereferencing.

class Simple {
    public int x;

    public constructor(int x) {
        this.x = x;
    }
}

Simple a = new Simple(1);
Simple b = new Simple(1);
Simple c = new Simple(2);

print("a equals b: " + a.equals(b));
print("a equals c: " + a.equals(c));

// Runtime-null reaches the synthesized equals body and is rejected by
// the isClassOf guard.
print("a equals null: " + a.equals(null));

// hashCode and toString
int h = a.hashCode();
print("a has hashCode: " + (h != 0));

string s = a.toString();
print("a toString not empty: " + (s != ""));
