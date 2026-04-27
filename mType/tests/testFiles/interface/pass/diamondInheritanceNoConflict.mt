// Diamond interface inheritance: D implements B and C, both extending A.
// No method conflict because A is empty (a marker). D should be usable
// as A, B, and C.

interface A {}
interface B extends A {}
interface C extends A {}

class D implements B, C {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

D d = new D(7);

print("d isClassOf A: " + (d isClassOf A));
print("d isClassOf B: " + (d isClassOf B));
print("d isClassOf C: " + (d isClassOf C));

A asA = d;
B asB = d;
C asC = d;

print("asA matches D: " + (asA isClassOf D));
print("asB matches D: " + (asB isClassOf D));
print("asC matches D: " + (asC isClassOf D));
print("d.value: " + d.value);
