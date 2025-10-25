// ERROR: Break in lambda without loop context

interface Processor {
    function process(): string;
}

function main(): void {
    // ERROR: Break outside of loop context
    Processor processor = () -> {
        break;  // ERROR: No loop to break from
        return "This should not execute";
    };

    processor.process();
    print("This should not execute");
}

main();
