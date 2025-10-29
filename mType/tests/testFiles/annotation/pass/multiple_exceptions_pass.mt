// Test: @Throw annotation with multiple exception classes
// Expected: Should compile and validate successfully
import * from "../../lib/exceptions/Exception.mt";
class IOException extends Exception {
    constructor(string message): super(message) {
    }
}

class NetworkException extends Exception {
    constructor(string message): super(message) {
    }
}

@Throw(IOException, NetworkException)
function fetchData(string url): string {
    // This function declares it may throw either IOException or NetworkException
    return "data";
}

// Test execution
string data = fetchData("http://example.com");
print("Success: " + data);
