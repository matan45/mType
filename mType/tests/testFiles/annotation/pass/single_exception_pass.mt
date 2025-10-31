// Test: Basic @Throw annotation with single exception class
// Expected: Should compile and validate successfully
import * from "../../lib/exceptions/Exception.mt";
class IOException extends Exception {
    constructor(string message): super(message) {
    }
}

@Throw(IOException)
function readFile(string path): string {
    // This is valid - function declares it may throw IOException
    return "file contents";
}

// Test execution
string result = readFile("test.txt");
print("Success: " + result);
