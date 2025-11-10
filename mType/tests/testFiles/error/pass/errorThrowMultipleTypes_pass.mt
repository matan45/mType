// Test: @Throw annotation with multiple exception types
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";
import * from "../../lib/exceptions/IllegalArgumentException.mt";

class IOException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class NetworkException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

// Function that declares multiple exception types
@Throw(IOException, NetworkException, IllegalArgumentException)
function fetchDataWithValidation(string url, int timeout): string {
    print("Fetching data from: " + url);

    if (timeout < 0) {
        throw new IllegalArgumentException("Timeout cannot be negative");
    }

    if (url == "invalid") {
        throw new IOException("Invalid URL provided");
    }

    if (url == "network-error") {
        throw new NetworkException("Network connection failed");
    }

    return "Data fetched successfully";
}

// Test case 1: Valid call
function testValidCall(): void {
    try {
        string result = fetchDataWithValidation("http://example.com", 5000);
        print("Result: " + result);
    } catch (Exception e) {
        print("Unexpected error: " + e.getMessage());
    }
}

// Test case 2: IOException
function testIOException(): void {
    try {
        string result = fetchDataWithValidation("invalid", 5000);
        print("Should not reach here");
    } catch (IOException e) {
        print("Caught IOException: " + e.getMessage());
    }
}

// Test case 3: NetworkException
function testNetworkException(): void {
    try {
        string result = fetchDataWithValidation("network-error", 5000);
        print("Should not reach here");
    } catch (NetworkException e) {
        print("Caught NetworkException: " + e.getMessage());
    }
}

// Test case 4: IllegalArgumentException
function testIllegalArgumentException(): void {
    try {
        string result = fetchDataWithValidation("http://example.com", -1);
        print("Should not reach here");
    } catch (IllegalArgumentException e) {
        print("Caught IllegalArgumentException: " + e.getMessage());
    }
}

print("=== Test 1: Valid call ===");
testValidCall();

print("");
print("=== Test 2: IOException ===");
testIOException();

print("");
print("=== Test 3: NetworkException ===");
testNetworkException();

print("");
print("=== Test 4: IllegalArgumentException ===");
testIllegalArgumentException();

print("");
print("All tests passed!");
