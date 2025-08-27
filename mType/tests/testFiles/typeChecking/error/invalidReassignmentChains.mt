class TypeA {
    string value;
    constructor(string v) {
        value = v;
    }
}

class TypeB {
    int number;
    constructor(int n) {
        number = n;
    }
}

class TypeC {
    bool flag;
    constructor(bool f) {
        flag = f;
    }
}

// Test invalid chained assignment with incompatible types
TypeA a1 = new TypeA("A1");
TypeA a2 = new TypeA("A2");
TypeB b1 = new TypeB(1);

// This should fail - TypeB cannot be assigned to TypeA
a1 = a2 = b1;