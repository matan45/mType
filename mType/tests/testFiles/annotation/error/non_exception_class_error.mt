// Test: @Throw annotation with class that doesn't extend Exception
// Expected: Should fail with "Class 'String' must extend Exception"

class MyRegularClass {
    constructor() {}
}

@Throw(exceptions = [MyRegularClass])
function doSomething(): void {
    // MyRegularClass doesn't extend Exception, so this should fail
}

// This should never execute due to compilation error
doSomething();
