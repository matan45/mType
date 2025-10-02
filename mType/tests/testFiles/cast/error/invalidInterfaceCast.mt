// Error: Invalid interface cast
interface I1 {}
interface I2 {}
class C implements I1 {}

C obj = new C();
I2 i2 = (I2)obj;  // Error: C doesn't implement I2
