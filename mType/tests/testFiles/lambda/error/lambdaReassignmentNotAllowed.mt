// Test that lambda reassignment is not allowed (by design)

interface Processor {
    function process(int value): int;
}

function main(): void {
    // Initial lambda assignment - this should work
    Processor proc = x -> { return x * 2; };

    // Attempt to reassign lambda - this should fail
    proc = x -> { return x + 5; };

    print("This should not be reached");
}

main();