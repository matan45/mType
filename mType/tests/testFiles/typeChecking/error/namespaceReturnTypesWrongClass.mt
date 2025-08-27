namespace ns1 {
    class TypeA {
        int value;
        constructor(int v) { value = v; }
    }
    
    function createA(): ns1::TypeA {
        return new ns1::TypeA(10);
    }
}

namespace ns2 {
    class TypeB {
        int value;
        constructor(int v) { value = v; }
    }
}

ns2::TypeB wrongType = ns1::createA();  // Should fail