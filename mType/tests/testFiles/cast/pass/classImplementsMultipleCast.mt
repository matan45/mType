// Test: Class implements multiple, cast between them
interface I1 { void m1(); }
interface I2 { void m2(); }

class C implements I1, I2 {
    void m1() { print("M1"); }
    void m2() { print("M2"); }
}

C obj = new C();
I1 i1 = (I1)obj;
I2 i2 = (I2)i1;  // Cast from one interface to another via same object
i2.m2();

// Expected output:
// M2
