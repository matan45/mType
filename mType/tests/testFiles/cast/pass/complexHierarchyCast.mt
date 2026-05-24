// Test: cast across a 4-level hierarchy; verify polymorphic dispatch and
// per-level field access still resolve correctly through each cast.
class A {
    public string fieldA = "A-field";
    public function show(): void { print("A.show"); }
}

class B extends A {
    public string fieldB = "B-field";
    public function show(): void { print("B.show"); }
}

class C extends B {
    public string fieldC = "C-field";
    public function show(): void { print("C.show"); }
}

class D extends C {
    public string fieldD = "D-field";
    public function show(): void { print("D.show"); }
}

D leaf = new D();
A asA = (A)leaf;
asA.show();
print(asA.fieldA);

C asC = (C)asA;
asC.show();
print(asC.fieldC);

D backToD = (D)asC;
backToD.show();
print(backToD.fieldD);

// Expected output:
// D.show
// A-field
// D.show
// C-field
// D.show
// D-field
