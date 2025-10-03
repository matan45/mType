// Test: Class implements multiple, cast between them
interface I1 { function m1() : void; }
interface I2 { function m2() : void; }

class C implements I1, I2 {
    function m1() : void { print("M1"); }
    function m2() : void { print("M2"); }
}

C obj = new C();
I1 i1 = (I1)obj;
I2 i2 = (I2)i1;  // Cast from one interface to another via same object
i2.m2();

// Expected output:
// M2
