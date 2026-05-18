// Test: `@Script` on a free function should be rejected.
// @Script is a class-level lifecycle marker — the validator looks for a
// default constructor and onStart/onUpdate/onDestroy methods on the class,
// none of which exist on a function. Allowed targets: [CLASS].

@Script
function main(): void {
    print("should not reach here");
}
