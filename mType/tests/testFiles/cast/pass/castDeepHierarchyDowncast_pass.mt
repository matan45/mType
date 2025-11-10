
// Test 5+ level deep hierarchy downcast
class A {
    public function identify(): string {
        return "A";
    }
}

class B extends A {
    public function identify(): string {
        return "B";
    }
}

class C extends B {
    public function identify(): string {
        return "C";
    }
}

class D extends C {
    public function identify(): string {
        return "D";
    }
}

class E extends D {
    public function identify(): string {
        return "E";
    }
}

class F extends E {
    public function identify(): string {
        return "F";
    }

    public function specialMethod(): string {
        return "Special F";
    }
}

function main(): void {
    F f = new F();
    A a = (A)f;  // Upcast to top

    print(a.identify());  // Should still use F's implementation

    // Downcast through hierarchy
    B b = (B)a;
    print(b.identify());

    C c = (C)b;
    print(c.identify());

    D d = (D)c;
    print(d.identify());

    E e = (E)d;
    print(e.identify());

    F backToF = (F)e;
    print(backToF.identify());
    print(backToF.specialMethod());

    // Direct downcast from top to bottom
    F directF = (F)a;
    print(directF.specialMethod());
}

main();
