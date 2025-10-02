// Test: Valid interface usage (error test is separate)
interface I { void m(); }
class C implements I { void m() { print("OK"); } }

C obj = new C();
I i = obj;
C back = (C)i;
back.m();

// Expected output:
// OK
