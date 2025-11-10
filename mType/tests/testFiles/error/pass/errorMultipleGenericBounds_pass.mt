// Test: Exception with multiple generic constraints
// Expected: Should compile and run successfully
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/int.mt";
import * from "../../lib/primitives/string.mt";

// Multiple interfaces for constraints
interface Serializable {
    public function serialize(): string;
}

interface Comparable<T> {
    public function compareTo(T other): int;
}

interface Identifiable {
    public function getId(): int;
}

// Class implementing multiple interfaces
class ErrorRecord implements Serializable, Comparable<ErrorRecord>, Identifiable {
    private int id;
    private string severity;
    private int timestamp;

    public constructor(int errorId, string sev, int time) {
        id = errorId;
        severity = sev;
        timestamp = time;
    }

    public function serialize(): string {
        return "ErrorRecord[" + id + "," + severity + "," + timestamp + "]";
    }

    public function compareTo(ErrorRecord other): int {
        if (timestamp < other.timestamp) {
            return -1;
        } else if (timestamp > other.timestamp) {
            return 1;
        }
        return 0;
    }

    public function getId(): int {
        return id;
    }

    public function getSeverity(): string {
        return severity;
    }

    public function getTimestamp(): int {
        return timestamp;
    }
}

// Generic exception with multiple bounds
class ConstrainedException<T extends Serializable> extends Exception {
    public T data;

    public constructor(string msg, T contextData): super(msg) {
        data = contextData;
    }

    public function getData(): T {
        return data;
    }

    public function getSerializedData(): string {
        return data.serialize();
    }
}

// Exception with more specific bounds
class TrackedException<T extends Identifiable> extends Exception {
    public T trackingData;

    public constructor(string msg, T tracking): super(msg) {
        trackingData = tracking;
    }

    public function getTrackingId(): int {
        return trackingData.getId();
    }

    public function getTrackingData(): T {
        return trackingData;
    }
}

// Function that throws constrained exception
function reportError(int errorId, string severity): void {
    ErrorRecord record = new ErrorRecord(errorId, severity, 1699000000);
    throw new ConstrainedException<ErrorRecord>("System error occurred", record);
}

// Function that throws tracked exception
function trackError(ErrorRecord record): void {
    throw new TrackedException<ErrorRecord>("Tracked error", record);
}

// Test multiple generic bounds with Serializable
print("Testing exception with Serializable constraint:");
try {
    reportError(500, "HIGH");
} catch (ConstrainedException<ErrorRecord> e) {
    print("Caught: " + e.getMessage());
    print("Serialized: " + e.getSerializedData());
    ErrorRecord rec = e.getData();
    print("Severity: " + rec.getSeverity());
}

// Test multiple generic bounds with Identifiable
print("Testing exception with Identifiable constraint:");
try {
    ErrorRecord rec = new ErrorRecord(404, "MEDIUM", 1699000100);
    trackError(rec);
} catch (TrackedException<ErrorRecord> e) {
    print("Caught: " + e.getMessage());
    print("Tracking ID: " + e.getTrackingId());
    ErrorRecord data = e.getTrackingData();
    print("Severity: " + data.getSeverity());
    print("Timestamp: " + data.getTimestamp());
}

// Test with comparable functionality
print("Testing comparable constraint:");
try {
    ErrorRecord rec1 = new ErrorRecord(100, "LOW", 1699000200);
    ErrorRecord rec2 = new ErrorRecord(101, "LOW", 1699000300);
    int comparison = rec1.compareTo(rec2);
    if (comparison < 0) {
        throw new ConstrainedException<ErrorRecord>("Earlier error found", rec1);
    }
} catch (ConstrainedException<ErrorRecord> e) {
    print("Caught: " + e.getMessage());
    print("Error ID: " + e.getData().getId());
}

print("Multiple generic bounds test passed!");
