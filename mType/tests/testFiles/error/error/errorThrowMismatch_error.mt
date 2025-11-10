// Error: Function throws exception not declared in @Throw annotation
// Expected: Compilation error - thrown exception type mismatch
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/exceptions/RuntimeException.mt";

class IOException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class NetworkException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

class DatabaseException extends Exception {
    public constructor(string msg) : super(msg) {
    }
}

// ERROR: Function declares IOException but throws DatabaseException
@Throw(IOException, NetworkException)
function fetchAndStore(string url): void {
    print("Fetching data from: " + url);

    if (url == "invalid") {
        throw new IOException("Invalid URL");
    }

    // ERROR: DatabaseException is not declared in @Throw annotation
    throw new DatabaseException("Failed to store data");
}

// This should not compile
fetchAndStore("http://example.com");
