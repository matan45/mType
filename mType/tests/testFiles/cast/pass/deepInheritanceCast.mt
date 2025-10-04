// Test: Deep inheritance hierarchy casting
class A {
    public int a = 1;
}

class B extends A {
    public int b = 2;
}

class C extends B {
    public int c = 3;
}

class D extends C {
    public int d = 4;
}

D objD = new D();
A objA = (A)objD;
print(objA.a);  // Expected: 1

C objC = (C)objA;
print(objC.c);  // Expected: 3

D castedD = (D)objC;
print(castedD.d);  // Expected: 4

// Expected output:
// 1
// 3
// 4
