namespace ns1 {
    class TypeA {
        int x;
    }
}

namespace ns2 {
    class TypeB {
        int y;
    }
    
    function process(ns1::TypeA obj): void {
        print("Processing TypeA");
    }
}

ns2::TypeB objB = new ns2::TypeB();
ns2::process(objB);  // Should fail: TypeB passed where TypeA expected