// Comprehensive test for closure capture edge cases and semantics

interface Function {
    function apply(int x): int;
}

interface Supplier {
    function get(): int;
}

interface Action {
    function execute(): void;
}

// Test 1: Static class member capture edge cases
class StaticCapture {
    static int globalCounter = 100;
    static final int CONSTANT = 42;
    int instanceField = 10;

    static function testStaticCapture(): void {
        print("=== Static Member Capture Edge Cases ===");

        // Static method capturing static field (should work)
        Function staticToStatic = x -> x + globalCounter;
        print("Static to static: " + staticToStatic.apply(5)); // Should be 105

        // Static field modified after lambda creation
        globalCounter = 200;
        print("After modification: " + staticToStatic.apply(5)); // Tests capture-by-value vs reference

        // Capture final static field
        Function finalCapture = x -> x * CONSTANT;
        print("Final capture: " + finalCapture.apply(2)); // Should be 84
    }

    function testInstanceToStatic(): void {
        print("=== Instance Method Capturing Static ===");

        // Instance method capturing static field
        Function instanceToStatic = x -> x + globalCounter + instanceField;
        print("Instance to static: " + instanceToStatic.apply(1));

        // Modify both after capture
        globalCounter = 300;
        instanceField = 20;
        print("After dual modification: " + instanceToStatic.apply(1));
    }
}

// Test 2: Nested lambda capture scenarios
class NestedCapture {
    int outerField = 50;

    function testNestedLambdaCapture(): void {
        print("=== Nested Lambda Capture ===");

        int outerLocal = 15;

        // Outer lambda captures both field and local
        Function outerLambda = x -> {
            int innerLocal = 25;

            // Inner lambda captures variables from multiple scopes
            Function innerLambda = y -> y + x + outerLocal + innerLocal + outerField;

            return innerLambda.apply(x + 1);
        };

        print("Nested capture result: " + outerLambda.apply(3));

        // Test capture chain
        Function createChainedLambda = base -> {
            Function level1 = a -> {
                Function level2 = b -> {
                    return a + b + base + outerField;
                };
                return level2.apply(a + 1);
            };
            return level1.apply(base * 2);
        };

        print("Chained capture: " + createChainedLambda.apply(2));
    }
}

// Test 3: Mutable vs Immutable capture semantics
class MutabilityCapture {
    function testMutableCapture(): void {
        print("=== Mutable vs Immutable Capture ===");

        int mutableVar = 10;
        final int immutableVar = 20;

        // Create lambda that captures both
        Supplier capturedSupplier = () -> mutableVar + immutableVar;

        print("Initial capture: " + capturedSupplier.get()); // Should be 30

        // Modify mutable variable after capture
        mutableVar = 100;
        print("After mutable change: " + capturedSupplier.get()); // Test capture semantics

        // Test capture of modified variables in loops
        int loopAccumulator = 0;
        for (int i = 0; i < 3; i++) {
            loopAccumulator = loopAccumulator + i;

            // Lambda capturing loop-modified variable
            Function loopCapture = x -> x + loopAccumulator + i;
            print("Loop iteration " + i + ": " + loopCapture.apply(5));
        }
    }
}

// Test 4: Complex capture scenarios
class ComplexCapture {
    static int staticCounter = 0;
    int instanceId;

    constructor(int id) {
        instanceId = id;
        staticCounter = staticCounter + 1;
    }

    function testComplexCapture(): void {
        print("=== Complex Capture Scenarios ===");

        int localValue = instanceId * 10;

        // Lambda capturing static, instance, and local variables
        Action complexAction = () -> {
            print("Complex capture - Static: " + staticCounter +
                  ", Instance: " + instanceId +
                  ", Local: " + localValue);
        };

        complexAction.execute();

        // Create another instance to test static counter changes
        ComplexCapture other = new ComplexCapture(999);
        print("After creating another instance, static counter: " + staticCounter);

        // Execute lambda again to test if static capture reflects changes
        complexAction.execute();
    }
}

// Test 5: Capture across different scopes and contexts
function testScopeCapture(): void {
    print("=== Scope Capture Edge Cases ===");

    int functionLocal = 77;

    // Function scope lambda
    Function funcScope = x -> x + functionLocal;
    print("Function scope: " + funcScope.apply(3));

    // Block scope capture
    if (true) {
        int blockLocal = 88;
        Function blockScope = x -> x + functionLocal + blockLocal;
        print("Block scope: " + blockScope.apply(2));
    }

    // Try-catch like scope (if supported)
    {
        int nestedBlockLocal = 99;
        Function nestedBlock = x -> x + functionLocal + nestedBlockLocal;
        print("Nested block: " + nestedBlock.apply(1));
    }
}

function main(): void {
    print("=== Closure Capture Edge Cases and Semantics Test ===");

    // Test static capture edge cases
    StaticCapture::testStaticCapture();

    StaticCapture instance = new StaticCapture();
    instance.testInstanceToStatic();

    // Test nested lambda captures
    NestedCapture nested = new NestedCapture();
    nested.testNestedLambdaCapture();

    // Test mutability semantics
    MutabilityCapture mutability = new MutabilityCapture();
    mutability.testMutableCapture();

    // Test complex scenarios
    ComplexCapture complex1 = new ComplexCapture(1);
    complex1.testComplexCapture();

    // Test scope captures
    testScopeCapture();

    print("=== All Closure Capture Edge Cases Completed ===");
}

main();