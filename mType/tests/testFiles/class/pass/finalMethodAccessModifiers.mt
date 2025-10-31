// Test: Final methods with different access modifiers
// Expected: Pass - final works with public, protected, and private

class Base {
    public final function publicFinal(): string {
        return "Public final method";
    }

    protected final function protectedFinal(): string {
        return "Protected final method";
    }

    private final function privateFinal(): string {
        return "Private final method";
    }

    public function callPrivateFinal(): string {
        return this.privateFinal();
    }
}

class Derived extends Base {
    // Cannot override any final methods regardless of access modifier

    public function testProtected(): string {
        // Can access protected final method from parent
        return this.protectedFinal();
    }

    public function testPublic(): string {
        // Can access public final method from parent
        return this.publicFinal();
    }
}

// Test different access modifiers with final
Base base = new Base();
print(base.publicFinal());  // Should print: Public final method
print(base.callPrivateFinal());  // Should print: Private final method

Derived derived = new Derived();
print(derived.publicFinal());  // Should print: Public final method (inherited)
print(derived.testProtected());  // Should print: Protected final method
print(derived.testPublic());  // Should print: Public final method
print(derived.callPrivateFinal());  // Should print: Private final method (inherited method calls private)
