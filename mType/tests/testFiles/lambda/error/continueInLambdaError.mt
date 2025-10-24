// ERROR: Continue in lambda without loop context

interface Processor {
    function process(): string;
}

function main(): void {
    // ERROR: Continue outside of loop context
    Processor processor = () -> {
        continue;  // ERROR: No loop to continue
        return "This should not execute";
    };

    processor.process();
    print("This should not execute");
}

main();
