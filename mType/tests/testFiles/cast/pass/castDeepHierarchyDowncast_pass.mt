@Script
// Test 5+ level deep hierarchy downcast
class A {
    fn identify(): String {
        return "A";
    }
}

class B : A {
    fn identify(): String {
        return "B";
    }
}

class C : B {
    fn identify(): String {
        return "C";
    }
}

class D : C {
    fn identify(): String {
        return "D";
    }
}

class E : D {
    fn identify(): String {
        return "E";
    }
}

class F : E {
    fn identify(): String {
        return "F";
    }

    fn specialMethod(): String {
        return "Special F";
    }
}

fn main() {
    let f: F = new F();
    let a: A = f as A;  // Upcast to top

    print(a.identify());  // Should still use F's implementation

    // Downcast through hierarchy
    let b: B = a as B;
    print(b.identify());

    let c: C = b as C;
    print(c.identify());

    let d: D = c as D;
    print(d.identify());

    let e: E = d as E;
    print(e.identify());

    let backToF: F = e as F;
    print(backToF.identify());
    print(backToF.specialMethod());

    // Direct downcast from top to bottom
    let directF: F = a as F;
    print(directF.specialMethod());
}
