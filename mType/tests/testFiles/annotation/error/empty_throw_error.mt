// Test: @Throw annotation with empty parameter list
// Expected: Should fail with parse error "Empty annotation parameters not allowed"

@Throw()
function doSomething(): void {
    // Empty @Throw is not allowed
}

// This should never execute due to compilation error
doSomething();
