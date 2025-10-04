// Test: Valid interface usage (error test is separate)
interface I { function m(): void; }
class C implements I { public function m(): void { print("OK"); } }

C obj = new C();
I i = obj;
C back = (C)i;
back.m();

// Expected output:
// OK
