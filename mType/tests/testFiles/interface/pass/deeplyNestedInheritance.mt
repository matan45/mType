// Test deeply nested interface inheritance (15 levels deep)
// This tests the interface validation caching and depth limits

interface Level1 {
    function method1(): void;
}

interface Level2 extends Level1 {
    function method2(): void;
}

interface Level3 extends Level2 {
    function method3(): void;
}

interface Level4 extends Level3 {
    function method4(): void;
}

interface Level5 extends Level4 {
    function method5(): void;
}

interface Level6 extends Level5 {
    function method6(): void;
}

interface Level7 extends Level6 {
    function method7(): void;
}

interface Level8 extends Level7 {
    function method8(): void;
}

interface Level9 extends Level8 {
    function method9(): void;
}

interface Level10 extends Level9 {
    function method10(): void;
}

interface Level11 extends Level10 {
    function method11(): void;
}

interface Level12 extends Level11 {
    function method12(): void;
}

interface Level13 extends Level12 {
    function method13(): void;
}

interface Level14 extends Level13 {
    function method14(): void;
}

interface Level15 extends Level14 {
    function method15(): void;
}

// Class implementing the deepest interface
class DeepImplementation implements Level15 {
    public function method1(): void { print("Method 1"); }
    public function method2(): void { print("Method 2"); }
    public function method3(): void { print("Method 3"); }
    public function method4(): void { print("Method 4"); }
    public function method5(): void { print("Method 5"); }
    public function method6(): void { print("Method 6"); }
    public function method7(): void { print("Method 7"); }
    public function method8(): void { print("Method 8"); }
    public function method9(): void { print("Method 9"); }
    public function method10(): void { print("Method 10"); }
    public function method11(): void { print("Method 11"); }
    public function method12(): void { print("Method 12"); }
    public function method13(): void { print("Method 13"); }
    public function method14(): void { print("Method 14"); }
    public function method15(): void { print("Method 15"); }
}

function main(): void {
    print("Testing deeply nested interface inheritance...");

    DeepImplementation impl = new DeepImplementation();

    // Test that all inherited methods are accessible
    impl.method1();
    impl.method5();
    impl.method10();
    impl.method15();

    print("Deep inheritance test completed successfully!");
}

main();
