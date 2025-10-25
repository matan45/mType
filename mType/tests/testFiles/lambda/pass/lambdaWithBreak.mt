// Test lambda containing loop with break

interface Processor {
    function process(): string;
}

function main(): void {
    print("Testing lambda with break");

    // Lambda containing loop with break
    Processor processor = () -> {
        string result = "";
        int count = 0;

        while (true) {
            if (count >= 5) {
                break;
            }
            result = result + count;
            count = count + 1;
        }

        return result;
    };

    print("Lambda result: " + processor.process());

    // Lambda with for loop and break
    Processor processor2 = () -> {
        string result = "Items: ";

        for (int i = 0; i < 10; i = i + 1) {
            if (i == 3) {
                break;
            }
            result = result + i + " ";
        }

        return result;
    };

    print("Lambda2 result: " + processor2.process());

    print("Lambda with break test completed");
}

main();
