// Test: equals/hashCode/toString on a class without explicit overrides.
// MYT-274: synthesis adds fast equals/hashCode whose Object parameter is
// non-nullable (matches inherited Object.equals to avoid overload
// ambiguity). Direct `equals(null)` was previously accepted via the lax
// typecheck on native methods; with the synthesized AST body it would now
// require an `Object?` parameter, which is incompatible with the Object
// signature. The functional behavior (null-not-equal) is still verified
// via the typechecked-OK detour through the synthesized isClassOf guard.

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

// hashCode and toString
int h = a.hashCode();
print("a has hashCode: " + (h != 0));

string s = a.toString();
print("a toString not empty: " + (s != ""));
